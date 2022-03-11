#include "sassy.h"
#include "tinyfiledialogs.h"

using namespace EvalFunc;

int is_value_prefix(char c)
{
    switch (c)
    {
    case '=':
    case '&':
    case '%':
    case '#':
        return 1;
    }
    return 0;
}

int is_active(int cell)
{
    for (int i = 0; i < gActivecells; i++)
        if (gActivecell[i] == cell)
            return 1;
    return 0;
}

void remove_active(int cell)
{
    if (!is_active(cell)) return;
    int i = 0;
    while (i < gActivecells && gActivecell[i] != cell) i++;
    if (gActivecell[i] == cell)
    {
        gActivecell[i] = gActivecell[gActivecells - 1];
        gActivecells--;
    }
}

void add_active(int cell)
{
    if (is_active(cell)) return;
    gActivecell[gActivecells] = cell;
    gActivecells++;
}

void setcellvalue(int cell, double value)
{
    // Keep existing prefix
    char prefix[2];
    prefix[0] = 0;
    prefix[1] = 0;
    if (is_value_prefix(gCelldata[cell].mRawtext[0]))
        prefix[0] = gCelldata[cell].mRawtext[0];

    gCelldata[cell].init();
    add_active(cell);
    sprintf(gCelldata[cell].mRawtext, "%s%3.7f", prefix, value);
    gCellvaluew[cell] = value;
}

void poke(int cell)
{
    delete[] gCelldata[cell].mDynmem;
    gCelldata[cell].mDynmem = 0;
    delete[] gCelldata[cell].mCode;
    gCelldata[cell].mCode = 0;
    gCelldata[cell].mDynamic = true;
    add_active(cell);
}

void rebuild();

void cold_reset()
{
    SDL_LockMutex(gAudioMutex);
    nukeResources();
    gActivecells = 0;
    for (int i = 0; i < MAXVAR; i++)
    {
        gCelldata[i].init();
        gCellvaluew[i] = 0;
        gCellvalue[i] = 0;
        add_active(i);
    }
    rebuild();
    SDL_UnlockMutex(gAudioMutex);
}

void warm_reset()
{
    SDL_LockMutex(gAudioMutex);
    gTime = 0;
    gActivecells = 0;
    for (int i = 0; i < MAXVAR; i++)
    {
        add_active(i);
        gCellvaluew[i] = 0;
        gCellvalue[i] = 0;
        if (gCelldata[i].mDynmem)
            memset(gCelldata[i].mDynmem, 0, gCelldata[i].mDynmemsize); 
        poke(i);
    }
    rebuild();
    gMidi_noteon = 0;
    for (int i = 0; i < 128; i++)
    {
        gMidi_poly_noteon[i] = 0;
        for (int j = 0; j < 16; j++)
            gMidi_perchan_noteon[j * 128 + i] = 0;
    }
    SDL_UnlockMutex(gAudioMutex);
}


void savefile()
{    
    if (!gConfig.mLastFilename)
        gConfig.mLastFilename = _strdup("");
    const char* filterpatterns[1] = { "*.tsf" };
    const char* fn = tinyfd_saveFileDialog("Save audio spreadsheet", gConfig.mLastFilename, 1,filterpatterns, "Tab separated file");
    if (!fn) return;
    free(gConfig.mLastFilename);
    gConfig.mLastFilename = _strdup(fn);

    SDL_LockMutex(gAudioMutex);
    FILE* f = fopen(fn, "wb");
    if (!f) return;
    for (int i = 0, c = 0; i < MAXROWS; i++)
    {
        for (int j = 0; j < 27; j++, c++)
        {
            fprintf(f, "%s\t", gCelldata[c].mRawtext);
        }
        fprintf(f, "\r\n");
    }
    fclose(f);
    SDL_UnlockMutex(gAudioMutex);
    gConfig.save();
}

void newfile()
{
    free(gConfig.mLastFilename);
    gConfig.mLastFilename = _strdup("");
    gConfig.save();
    cold_reset();
}

void loadfile(int reload)
{
    if (!gConfig.mLastFilename)
        gConfig.mLastFilename = _strdup("");
    
    const char* fn = gConfig.mLastFilename;

    if (!reload)
    {
        const char* filterpatterns[1] = { "*.tsf" };
        fn = tinyfd_openFileDialog("Open audio spreadsheet", gConfig.mLastFilename, 1, filterpatterns, "Tab separated file", 0);
        if (!fn) return;
    }

    FILE* f = fopen(fn, "rb");
    if (!f) return;

    SDL_LockMutex(gAudioMutex);
    cold_reset();

    if (!reload)
    {
        free(gConfig.mLastFilename);
        gConfig.mLastFilename = _strdup(fn);
    }
    int c = 0;
    int idx = 0;
    while (!feof(f) && c < MAXVAR)
    {
        int d = fgetc(f);
        if (d == '\t')
        {
            gCelldata[c].mRawtext[idx] = 0;
            c++;
            idx = 0;
        }
        else
        if (!feof(f))
        {
            if (idx == 0 && d == '\r' || d == '\n')
            {
            }
            else
            {
                gCelldata[c].mRawtext[idx] = (char)d;
                idx++;
            }
        }
    }
    gCelldata[c - 1].mRawtext[idx] = 0;

    fclose(f);
    SDL_UnlockMutex(gAudioMutex);    
    warm_reset();
    gConfig.save();
}

Xbyak::CodeGenerator gAsmcode(4096*256); // 1M of code ought to be enough for everybody..
jitfunc gJitcode;


void rebuild()
{
    jit_init(gAsmcode);
    for (int a = 0; a < gActivecells; a++)
    {
        int cell = gActivecell[a];
        gCelldata[cell].mLabel = gCelldata[cell].mDefaultLabel;

        if (gCelldata[cell].mRawtext[0] == '\'' || gCelldata[cell].mRawtext[0] == 0)
        {
            gCelldata[cell].mText = true;
            gCelldata[cell].mDynamic = false;
            gCellvaluew[cell] = nan("0");
            gCellvalue[cell] = nan("0");
            remove_active(cell);
            a--;
        }
        else
        {
            gCelldata[cell].mText = false;
            if (gCelldata[cell].mCode)
            {
                // Skip parse if we still have the bytecode
                jit_addcode(gCelldata[cell].mCode, cell, gAsmcode, gCelldata[cell].mTextStore);
            }
            else
            {
                parse(gCelldata[cell].mRawtext + ((is_value_prefix(gCelldata[cell].mRawtext[0])) ? 1 : 0), // skip initial '='/'&' etc if found
                    gBC,
                    gCelldata[cell].mTextStore); 

                if (gBC[1].mOpcode == 0 && gBC[0].mOpcode == 'C')
                {
                    gCellvaluew[cell] = gBC[0].mArg.f;
                }
                else
                {
                    if (lexify(gBC))
                    {
                        gCellvaluew[cell] = nan("0");
                        gBC[0].mOpcode = 'C';
                        gBC[1].mOpcode = 0;
                    }
                    else
                    {
                        opt(gBC);
                        if (gBC[1].mOpcode == 0 && gBC[0].mOpcode == 'C')
                        {
                            gCellvaluew[cell] = gBC[0].mArg.f;
                        }
                        else
                        {

                            int memneeded = postfix(gBC);
                            if (memneeded)
                            {
                                gCelldata[cell].mDynmem = new char[memneeded];
                                memset(gCelldata[cell].mDynmem, 0, memneeded);
                                gCelldata[cell].mDynmemsize = memneeded;
                            }

                            jit_addcode(gBC, cell, gAsmcode, gCelldata[cell].mTextStore);
                            // eliminate initial nan, helps making self-refers work
                            gCellvaluew[cell] = gCellvalue[cell] = 0;
                        }

                    }
                }

                // update usedby structures
                int i;
                for (i = 0; i < MAXVAR; i++)
                    gCelldata[i].mUsedby.erase(cell);
                i = 0;
                while (gBC[i].mOpcode != 0)
                {
                    if (gBC[i].mOpcode == 'V')
                        gCelldata[gBC[i].mArg.i.i - 1].mUsedby.insert(cell);
                    if (gBC[i].mOpcode == 'A')
                    {
                        int x0, y0, x1, y1;
                        cell_to_xy(gBC[i].mArg.i.i - 1, x0, y0);
                        cell_to_xy(gBC[i].mArg.i.j - 1, x1, y1);
                        if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
                        if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }
                        for (int y = y0; y <= y1; y++)
                            for (int x = x0; x <= x1; x++)
                                gCelldata[y * 27 + x].mUsedby.insert(cell);
                    }
                    i++;
                }

                // update uses structures
                gCelldata[cell].mUses.clear();
                gCelldata[cell].mUsesArea.clear();
                {
                    i = 0;
                    while (gBC[i].mOpcode != 0)
                    {
                        if (gBC[i].mOpcode == 'V')
                            gCelldata[cell].mUses.insert(gBC[i].mArg.i.i - 1);
                        if (gBC[i].mOpcode == 'A')
                        {
                            int x0, y0, x1, y1;
                            cell_to_xy(gBC[i].mArg.i.i - 1, x0, y0);
                            cell_to_xy(gBC[i].mArg.i.j - 1, x1, y1);
                            if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
                            if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }
                            gCelldata[cell].mUsesArea.insert(std::pair<int, int>(y0 * 27 + x0, y1 * 27 + x1));
                        }

                        i++;
                    }
                }

                if (gBC[0].mOpcode == 'C' && gBC[1].mOpcode == 0)
                {
                    gCelldata[cell].mDynamic = false;
                    gCellvalue[cell] = gCellvaluew[cell];
                    remove_active(cell);
                    a--;
                }
                else
                {
                    gCelldata[cell].mDynamic = true;
                    int l = 0;
                    while (gBC[l].mOpcode != 0) l++;
                    gCelldata[cell].mCode = new Op[l + 1];
                    memcpy(gCelldata[cell].mCode, gBC, sizeof(Op) * (l + 1));
                }
            }
        }
    }
    gJitcode = jit_finish(gAsmcode);
}


void eval_all()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // tick-tock cell value buffers
    double* t = gCellvalue;    
    gCellvalue = gCellvaluew;
    gCellvaluew = t;
    
    // time marches on
    gTime += dt();

    midiplayer_tick();

    // deal with midi out rate
    gMidiCredits += gConfig.mMidiRate * dt();
    if (gMidiCredits > 10) gMidiCredits = 10;
    
    // since probe has several channels, need to deal with index here
    if (gScope->mMode == 0)
    {        
        gScope->mCh[0].mData[gScope->mIndex] = gProbe[0];
        gScope->mCh[1].mData[gScope->mIndex] = gProbe[1];
        gScope->mCh[2].mData[gScope->mIndex] = gProbe[2];
        gScope->mCh[3].mData[gScope->mIndex] = gProbe[3];
        gScope->mIndex = (gScope->mIndex + 1) % (gSamplerate * 10);
    }

    // let's hit it
    if (gJitcode != NULL)
        gJitcode(gCellvalue, gCellvaluew);

    // check if we need to send note offs
    // TODO: drum track?
    midioutscan();
    
    if (gLaunchpadMode)
        launchpad_update();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = end_time - start_time;
    gCPUTimePerSample = static_cast<double>(std::chrono::nanoseconds(duration).count());

}


volatile bool shutdown = false;

#ifdef DEBUG_CAPTURE_RESAMPLING
unsigned int total_reads = 0;
unsigned int total_writes = 0;
#endif

void sdl2_audiomixer(void* , Uint8* stream, int len)
{
    if (shutdown)
    {
        memset(stream, 0, len);
        return;
    }
    _controlfp(_DN_FLUSH, _MCW_DN);
    /*
    SetThreadAffinityMask(GetCurrentThread(), 0xfe); // Avoid code #1
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadPriorityBoost(GetCurrentThread(), true);
    */
    auto start_time = std::chrono::high_resolution_clock::now();
    SDL_LockMutex(gAudioMutex);

    float* buf = (float*)stream;
    int samples = len / (gActiveAudioSpec.channels * sizeof(float));

    if (gMuted)
    {
        for (int i = 0; i < samples; i++)
        {
            gInputSample[0] = gCaptureData[gCaptureRead * 2 + 0];
            gInputSample[1] = gCaptureData[gCaptureRead * 2 + 1];
            gCaptureRead = (gCaptureRead + 1) % CAPTURE_LEN;
            eval_all();
            buf[i * 2 + 0] = 0;
            buf[i * 2 + 1] = 0;
#ifdef DEBUG_CAPTURE_RESAMPLING
            total_reads++;
#endif
        }
    }
    else
    {
        for (int i = 0; i < samples; i++)
        {
            gInputSample[0] = gCaptureData[gCaptureRead * 2 + 0];
            gInputSample[1] = gCaptureData[gCaptureRead * 2 + 1];
            gCaptureRead = (gCaptureRead + 1) % CAPTURE_LEN;
            eval_all();
            buf[i * 2 + 0] = gOutputSample[0];
            buf[i * 2 + 1] = gOutputSample[1];
#ifdef DEBUG_CAPTURE_RESAMPLING
            total_reads++;
#endif
        }
    }

    SDL_UnlockMutex(gAudioMutex);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = end_time - start_time;
    float generated_time = ((float)samples / (float)gActiveAudioSpec.freq) * 1000 * 1000;
    if (generated_time)
    {
        float cpu_use = ((float)(duration / (std::chrono::microseconds(1))) / generated_time) * 100.0f;
        gCPU = cpu_use;
        gCPU_peak = (gCPU_peak * 99 + gCPU) / 100.0f;
        if (gCPU_peak < cpu_use)
        {            
            gCPU_peak = cpu_use;
        }
        gCPU_avg_sum -= gCPU_avg_data[gCPU_avg_idex];
        gCPU_avg_sum += gCPU;
        gCPU_avg_data[gCPU_avg_idex] = gCPU;
        gCPU_avg_idex++;
        gCPU_avg_idex %= AVG_SAMPLES;
    }
}

float gCaptureResampleBuffer[8192];

void sdl2_audiocapture(void*, Uint8* stream, int len)
{
    SDL_LockMutex(gAudioMutex);
    float* buf = (float*)stream;
    int samples = len / (sizeof(float) * 2);
    if (gSamplerate == gActiveCaptureSpec.freq)
    {
        // No resampling needed, yay
        for (int i = 0; i < samples; i++)
        {
            gCaptureData[gCaptureWrite * 2 + 0] = buf[i * 2 + 0];
            gCaptureData[gCaptureWrite * 2 + 1] = buf[i * 2 + 1];
            gCaptureWrite = (gCaptureWrite + 1) % CAPTURE_LEN;
#ifdef DEBUG_CAPTURE_RESAMPLING
            total_writes++;
#endif
        }
    }
    else
    {
        SRC_DATA d;
        d.data_in = buf;
        d.input_frames = samples;
        d.data_out = gCaptureResampleBuffer;
        d.output_frames = 8192;
        d.src_ratio = (double)gSamplerate / (double)gActiveCaptureSpec.freq;
        d.end_of_input = 0;
        src_process(gCaptureResampler, &d);

        for (int i = 0; i < d.output_frames_gen; i++)
        {
            gCaptureData[gCaptureWrite * 2 + 0] = gCaptureResampleBuffer[i * 2 + 0];
            gCaptureData[gCaptureWrite * 2 + 1] = gCaptureResampleBuffer[i * 2 + 1];
            gCaptureWrite = (gCaptureWrite + 1) % CAPTURE_LEN;
#ifdef DEBUG_CAPTURE_RESAMPLING
            total_writes++;
#endif
        }
    }
    SDL_UnlockMutex(gAudioMutex);
#ifdef DEBUG_CAPTURE_RESAMPLING
    printf("\rw:%8d r:%8d d:%d", total_writes, total_reads, total_writes - total_reads);
#endif
}


bool match(char* a, char* b)
{
    while (*a)
    {
        if (toupper(*a) == toupper(*b))
        {
            a++; b++;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void updateuistore(int cell, int element, float v)
{
    char temp[1025];
    memcpy(temp, gCelldata[cell].mRawtext, 1025);
    char* s = temp;
    char* d = gCelldata[cell].mRawtext;
    int mpot = 0;
    // 1. find the correct element
    element++;
    while (element)
    {
        if (match("slider", s) 
         || match("midipot", s) 
         || match("sliderpot", s) 
         || match("toggle", s)
         || match("encoder", s))
        {
            mpot = match("midipot", s) || match("sliderpot", s);
            element--;
            if (element)
            {
                *d = *s;
                d++;
                s++;
            }
        }
        else
        {
            *d = *s;
            d++;
            s++;
        }
    }
    // 2. replace it
    while (*s != '(')
    {
        *d = *s;
        d++;
        s++;
    }
    *d = *s;
    d++;
    s++;
    d += sprintf(d, "%3.3f", v);
    if (mpot)
    {
        // Check if this is unmoved pot
        char* t = s;
        while (*t != ')' && *t != ',') t++;
        if (*t == ')') // yup, unmoved.
        {
            *d = ',';
            d++;
            while (*s != ')')
            {
                *d = *s;
                d++;
                s++;
            }
        }
    }
    while (*s != 0 && *s != ',' && *s != ')') s++;
    // 3. ?!
    while (*s != 0)
    {
        *d = *s;
        d++;
        s++;
    }
    // 4. profit
    *d = 0;
}



void cell_to_xy(int cell, int &x, int &y)
{
    x = cell % 27;
    y = cell / 27;
}

int is_area(int cell)
{
    if (gArea1 == -1) return 0;
    int x0, y0, x1, y1, x, y;
    cell_to_xy(cell, x, y);
    cell_to_xy(gArea1, x0, y0);
    cell_to_xy(gArea2, x1, y1);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    if (x >= x0 && x <= x1 && y >= y0 && y <= y1) return 1;
    return 0;
}

void do_del_area(int area1, int area2)
{
    SDL_LockMutex(gAudioMutex);

    int x0, y0, x1, y1;
    cell_to_xy(area1, x0, y0);
    cell_to_xy(area2, x1, y1);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    for (int i = x0; i <= x1; i++)
    {
        for (int j = y0; j <= y1; j++)
        {
            gCelldata[j * 27 + i].init();
            poke(j * 27 + i);
            for (int c = 0; c < MAXVAR; c++)
                gCelldata[c].mUsedby.erase(j * 27 + i);
        }
    }
    rebuild(); // poked

    SDL_UnlockMutex(gAudioMutex);
}

void del_area()
{
    int single = 0;
    if (gArea1 == -1)
    {
        gArea1 = gArea2 = gEditorcell;
        single = 1;
    }

    do_del_area(gArea1, gArea2);

    if (single)
        gArea1 = -1;
}


std::string do_copy_area(int area1, int area2)
{
    int x0, y0, x1, y1;
    cell_to_xy(area1, x0, y0);
    cell_to_xy(area2, x1, y1);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    gCopyofs = x0 + y0 * 27;

    std::string t;
    for (int j = y0; j <= y1; j++)
    {
        for (int i = x0; i <= x1; i++)
        {
            t.append(gCelldata[j * 27 + i].mRawtext);
            if (i != x1)
                t.append("\t");
        }
        t.append("\r\n");
    }
    return t;
}

void copy_area()
{
    int single = 0;
    if (gArea1 == -1)
    {
        gArea1 = gArea2 = gEditorcell;
        single = 1;
    }

    ImGui::SetClipboardText(do_copy_area(gArea1, gArea2).c_str());

    if (single)
        gArea1 = -1;
}

void do_paste_area(int area1, int area2, const char *t)
{
    int x0, y0, x1, y1;
    cell_to_xy(area1, x0, y0);
    cell_to_xy(area2, x1, y1);
    if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
    if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }
    std::string* data;
    int rows = 0, cols = 1;

    {
        const char* s = t;
        while (*s && *s != '\n')
        {
            if (*s == '\t')
                cols++;
            s++;
        }
        while (*s)
        {
            if (*s == '\n')
                rows++;
            s++;
        }
        if (rows == 0) rows = 1;
        data = new std::string[rows * cols];
        s = t;
        int idx = 0;
        while (*s)
        {
            if (*s == '\t' || *s == '\n') idx++;
            else if (*s == '\r');
            else
                data[idx] += *s;
            s++;
        }

        if (y1 - y0 < rows - 1) y1 = y0 + rows - 1;
        if (x1 - x0 < cols - 1) x1 = x0 + cols - 1;
        if (y1 > MAXROWS-1) y1 = MAXROWS-1;
        if (x1 > 26) x1 = 26;
    }
    int xofs = x0 - (gCopyofs % 27);
    int yofs = y0 - (gCopyofs / 27);

    SDL_LockMutex(gAudioMutex);

    for (int j = y0; j <= y1; j++)
    {
        for (int i = x0; i <= x1; i++)
        {
            int srcidx = ((j - y0) % rows) * cols + (i - x0) % cols;
            int srcyofs = (j - y0) / rows;
            int srcxofs = (i - x0) / cols;
            std::string transformed;
            const char* src = data[srcidx].c_str();
            while (*src)
            {
                std::string sym;
                if (isalpha(*src))
                {
                    while (isalpha(*src) || isnum(*src))
                    {
                        sym += *src;
                        src++;
                    }
                    if (is_symbol(sym.c_str()) > 0)
                    {
                        int v = is_symbol(sym.c_str()) - 1;
                        bool col_lock = (sym[0] == '$');
                        bool row_lock = (sym[col_lock?2:1] == '$');
                        int x = v % 27;
                        x += col_lock ? 0 : xofs + srcxofs;
                        int y = v / 27;
                        y += row_lock ? 0 : yofs + srcyofs;
                        char temp[16];
                        // sprintf(temp, "%s%c%s%d", col_lock ? "$" : "", 'A' + x, row_lock ? "$" : "", y + 1);
                        char* s = temp;
                        s += sprintf(s, "%s", col_lock ? "$" : "");
                        if (x >= 0 && x <= 26) s += sprintf(s, "%c", 'A' + x); else s += sprintf(s, "#REF!");
                        s += sprintf(s, "%s", row_lock ? "$" : "");
                        if (y >= 0 && y <= MAXROWS) s += sprintf(s, "%d", y + 1); else s += sprintf(s, "#REF!");
                        transformed += temp;
                    }
                    else
                    {
                        transformed += sym;
                    }
                }
                else
                {
                    transformed += *src;
                    src++;
                }
            }
            strcpy(gCelldata[j * 27 + i].mRawtext, transformed.c_str());
            poke(j * 27 + i);
        }
    }

    rebuild(); // poked

    SDL_UnlockMutex(gAudioMutex);

    delete[] data;
}

void paste_area()
{
    int single = 0;
    if (gArea1 == -1)
    {
        gArea1 = gArea2 = gEditorcell;
        single = 1;
    }

    do_paste_area(gArea1, gArea2, ImGui::GetClipboardText());

    if (single)
        gArea1 = -1;
}


void initcapture()
{
    if (gCaptureDeviceID)
    {
        SDL_CloseAudioDevice(gCaptureDeviceID);
    }
    SDL_AudioSpec as;
    memset(&as, 0, sizeof(SDL_AudioSpec));

    if (!gCaptureResampler)
        gCaptureResampler = src_new(gConfig.mCaptureResampler, 2, 0);
    else
        src_reset(gCaptureResampler);

    as.freq = gSamplerate;
    as.format = AUDIO_F32;
    as.channels = 2;
    as.samples = 2048; // sdl2 seems to ignore this anyway
    as.callback = sdl2_audiocapture;
    const char* devicename = NULL;
    if (gConfig.mConfig.has("general") && gConfig.mConfig["general"].has("input_device"))
        if (gConfig.mConfig["general"]["input_device"].length() > 0)
            devicename = gConfig.mConfig["general"]["input_device"].c_str();

    gCaptureRead = 0;
    gCaptureWrite = 0;
#ifdef DEBUG_CAPTURE_RESAMPLING
    total_reads = 0;
    total_writes = 0;
#endif
    if (devicename != NULL)
    {
        gCaptureDeviceID = SDL_OpenAudioDevice(devicename, 1, &as, &gActiveCaptureSpec, SDL_AUDIO_ALLOW_ANY_CHANGE & ~(SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
        if (gCaptureDeviceID == 0)
        {
            printf("Capture device creation failed: %s\n", SDL_GetError());
        }
        SDL_PauseAudioDevice(gCaptureDeviceID, 0);
    }
}

void initaudio()
{
    if (gAudioDeviceID)
    {
        SDL_CloseAudioDevice(gAudioDeviceID);
    }
    SDL_AudioSpec as;
    memset(&as, 0, sizeof(SDL_AudioSpec));

    as.freq = gSamplerate;
    as.format = AUDIO_F32;
    as.channels = 2;
    as.samples = 2048; // sdl2 seems to ignore this anyway
    as.callback = sdl2_audiomixer;
    const char* devicename = NULL;
    if (gConfig.mConfig.has("general") && gConfig.mConfig["general"].has("output_device"))
        if (gConfig.mConfig["general"]["output_device"].length() > 0)
            devicename = gConfig.mConfig["general"]["output_device"].c_str();

    gAudioDeviceID = SDL_OpenAudioDevice(devicename, 0, &as, &gActiveAudioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE & ~(SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE));
    if (gAudioDeviceID == 0)
    {
        printf("Audio device creation failed: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(gAudioDeviceID, 0);
}

void initasio()
{
    asio_deinit();
    if (gConfig.mConfig.has("general") && gConfig.mConfig["general"].has("asio_device"))
        if (gConfig.mConfig["general"]["asio_device"].length() > 0)
            asio_init((char*)gConfig.mConfig["general"]["asio_device"].c_str(), gSamplerate);
}


void initmidi()
{
    SDL_LockMutex(gAudioMutex);
    gLaunchpadMode = false;

    if (gRtmidi_in->isPortOpen())
    {
        gRtmidi_in->closePort();
        gRtmidi_in->cancelCallback();
    }

    if (gRtmidi_out->isPortOpen())
    {
        gRtmidi_out->closePort();
    }

    if (gConfig.mConfig.has("midi") && gConfig.mConfig["midi"].has("input_device") && gConfig.mConfig["midi"]["input_device"].length() > 0)
    {
        for (int i = 0; i < (signed)gRtmidi_in->getPortCount(); i++)
        {
            if (gRtmidi_in->getPortName(i).compare(gConfig.mConfig["midi"]["input_device"]) == 0)
            {
                gRtmidi_in->openPort(i);
                if (gRtmidi_in->isPortOpen())
                {
                    gRtmidi_in->setCallback(midicallback);
                    gRtmidi_in->ignoreTypes(true, true, false);
                }
            }
        }
    }

    if (gConfig.mConfig.has("midi") && gConfig.mConfig["midi"].has("output_device") && gConfig.mConfig["midi"]["output_device"].length() > 0)
    {
        for (int i = 0; i < (signed)gRtmidi_out->getPortCount(); i++)
        {
            if (gRtmidi_out->getPortName(i).compare(gConfig.mConfig["midi"]["output_device"]) == 0)
            {
                gRtmidi_out->openPort(i);
            }
        }
    }
    SDL_UnlockMutex(gAudioMutex);
}


ImVec2 editorcell_pos;
ImVec2 grid_min, grid_max;

void showusedby()
{
    int x0, y0, x1, y1;
    cell_to_xy(gEditorcell, x0, y0);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(grid_min, grid_max, true);
    for (auto it = gCelldata[gEditorcell].mUsedby.begin(); it != gCelldata[gEditorcell].mUsedby.end(); ++it)
    {
        ImVec2 coords0 = editorcell_pos;
        ImVec2 coords1 = editorcell_pos;
        int targetcell = *it;
        cell_to_xy(targetcell, x1, y1);
        coords1.x += (x1-x0) * 65 * gConfig.mUIScale;
        coords1.y += (y1-y0) * 18 * gConfig.mUIScale;
        coords0.x += 5 * gConfig.mUIScale;
        coords0.y += 2 * gConfig.mUIScale;
        
        dl->AddLine(coords0, coords1, 0x7f7fff3f, 4.0f);
    }
    dl->PopClipRect();
}

void showuses()
{
    int x0, y0, x1, y1, x, y;
    cell_to_xy(gEditorcell, x, y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(grid_min, grid_max, true);
    for (auto it = gCelldata[gEditorcell].mUses.begin(); it != gCelldata[gEditorcell].mUses.end(); ++it)
    {
        ImVec2 coords0 = editorcell_pos;
        ImVec2 coords1 = editorcell_pos;
        int targetcell = *it;
        cell_to_xy(targetcell, x1, y1);
        coords1.x += (x1 - x) * 65 * gConfig.mUIScale;
        coords1.y += (y1 - y) * 18 * gConfig.mUIScale;
        coords0.x += -5 * gConfig.mUIScale;
        coords0.y += -2 * gConfig.mUIScale;

        dl->AddLine(coords0, coords1, 0x7f7f3fff, 4.0f);
    }

    for (auto it = gCelldata[gEditorcell].mUsesArea.begin(); it != gCelldata[gEditorcell].mUsesArea.end(); ++it)
    {
        cell_to_xy(it->first, x0, y0);
        cell_to_xy(it->second, x1, y1);
        ImVec2 coords0 = editorcell_pos;
        ImVec2 coords3 = editorcell_pos;
        coords0.x += (x0 - x) * 65 - 20;
        coords0.y += (y0 - y) * 18;
        coords3.x += (x1 - x) * 65 + 20;
        coords3.y += (y1 - y) * 18;
        ImVec2 coords1, coords2;
        coords1.x = coords3.x;
        coords1.y = coords0.y;
        coords2.x = coords0.x;
        coords2.y = coords3.y;
        ImVec2 coordsc;
        coordsc.x = (coords0.x + coords3.x) / 2;
        coordsc.y = (coords0.y + coords3.y) / 2;

        dl->AddLine(coordsc, editorcell_pos, 0x7f7f3fff, 4.0f);
        dl->AddLine(coords0, coords2, 0x7f7f3fff, 4.0f);
        dl->AddLine(coords2, coords3, 0x7f7f3fff, 4.0f);
        dl->AddLine(coords3, coords1, 0x7f7f3fff, 4.0f);
        dl->AddLine(coords1, coords0, 0x7f7f3fff, 4.0f);
    }

    dl->PopClipRect();
}

extern int editor_callback(ImGuiInputTextCallbackData* data);

// Main code
int main(int, char**)
{
    printf("Initializing..\n");
    _controlfp(_DN_FLUSH, _MCW_DN);

    gCelldata = new Celldata[MAXVAR];
    gCellvalue = new double[MAXVAR];
    gCellvaluew = new double[MAXVAR];
    memset(gCellvalue, 0, sizeof(double) * MAXVAR);
    memset(gCellvaluew, 0, sizeof(double) * MAXVAR);

    for (int i = 0; i < 26; i++)
        for (int j = 0; j < MAXROWS; j++)
            sprintf(gCelldata[j * 27 + i].mDefaultLabel, "%c%d", 'A' + i, j + 1);

    gConfig.load();

    init_symbols();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    if (gConfig.mConfig.has("general") && gConfig.mConfig["general"].has("audio_driver") &&
        gConfig.mConfig["general"].get("audio_driver").length() > 0)
        SDL_AudioInit(gConfig.mConfig["general"].get("audio_driver").c_str());

    printf("Looking for midi devices..\n");
    try
    {
        gRtmidi_in = new RtMidiIn();
        gRtmidi_out = new RtMidiOut();
    }
    catch (RtMidiError& error)
    {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    printf("RTMidi reports %d input ports:\n", gRtmidi_in->getPortCount());
    for (int i = 0; i < (signed)gRtmidi_in->getPortCount(); i++)
        printf("%d: %s\n", i, gRtmidi_in->getPortName(i).c_str());

    printf("\nRTMidi reports %d output ports:\n", gRtmidi_out->getPortCount());
    for (int i = 0; i < (signed)gRtmidi_out->getPortCount(); i++)
        printf("%d: %s\n", i, gRtmidi_out->getPortName(i).c_str());

    int count = SDL_GetNumAudioDrivers();
    printf("\nSDL reports %d audio drivers:\n", count);
    for (int i = 0; i < count; i++) {
        printf("%d: %s\n", i, SDL_GetAudioDriver(i));
    }

    count = SDL_GetNumAudioDevices(0);

    printf("\nSDL reports %d audio devices:\n", count);
    for (int i = 0; i < count; ++i) {
        printf("%d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }
    
    count = SDL_GetNumAudioDevices(1);

    printf("\nSDL reports %d input devices:\n", count);
    for (int i = 0; i < count; ++i) {
        printf("%d: %s\n", i, SDL_GetAudioDeviceName(i, 1));
    }

    
    count = asio_devicecount();
    
    printf("\nASIO reports %d devices:\n", count);
    for (int i = 0; i < count; ++i) {
        printf("%d: %s\n", i, asio_devicename(i));
    }


    cold_reset();

    gAudioMutex = SDL_CreateMutex();
    gScope = new ScopeData;

    // Setup audio
    initaudio();
    initcapture();
    initmidi();
    initasio();

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Sassy audio spreadsheet v" SASSY_VERSION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Our state
    bool table_mode = false;
    bool show_demo_window = false;
    bool openabout = false;
    bool openconfig = false;
    bool focuseditor = false;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);    

    if (gConfig.mReloadLast)
        loadfile(1);


    // Main loop
    bool done = false;
    bool first_cycle = true;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        io.FontGlobalScale = gConfig.mUIScale;

        checkResourceRequests();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);       

        if (gShowScopeWindow)
            do_show_scope_window();

        if (gShowHelpWindow)
            do_show_help_window();

        if (gShowKeyboardWindow)
            do_show_keyboard_window();

        if (gShowMidiPlayerWindow)
            do_show_midiplayer_window();

        bool scroll_to_editorcell = false;

        ImVec2 mainwindoriginalsize = ImGui::GetMainViewport()->Size;
        mainwindoriginalsize.y -= (150+18) * gConfig.mUIScale;

        {         
            ImGui::SetNextWindowPos(ImVec2(0, 18 * gConfig.mUIScale));
            ImGui::SetNextWindowSize(mainwindoriginalsize);
            ImGui::Begin("Editor", 0, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            if (gConfig.mShowUsedBy && !(table_mode && gArea1 != -1))
                showusedby();
            if (gConfig.mShowUses && !(table_mode && gArea1 != -1))
                showuses();

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Save.."))
                    {
                        savefile();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Load.."))
                    {
                        loadfile(0);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Revert to saved"))
                    {
                        loadfile(1);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("New"))
                    {
                        newfile();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit"))
                    {
                        done = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Windows"))
                {
                    if (ImGui::MenuItem("Preferences..", 0))
                        openconfig = true;
                    ImGui::MenuItem("Scope", 0, &gShowScopeWindow);
                    ImGui::MenuItem("MIDI Keyboard", 0, &gShowKeyboardWindow);
                    ImGui::MenuItem("MIDI player", 0, &gShowMidiPlayerWindow);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Help"))
                {
                    ImGui::MenuItem("Function reference", 0, &gShowHelpWindow);
                    ImGui::Separator();
                    if (ImGui::MenuItem("About Sassy..", 0))
                        openabout = true;
#ifdef _DEBUG
                    ImGui::Separator();
                    ImGui::MenuItem("IMGUI demo", 0, &show_demo_window);
#endif
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }
            
            /*
            for (int i = 0; i < 512; i++)
                if (io.KeysDown[i])
                    printf("Key %d down\n", i);
                    */
            for (int i = 0; i < 100; i++)
                if (io.KeysDown[i])
                {
                    gWasdown[i] = true;
                    if (io.KeyCtrl) gWasdown[i + 100] = true;
                }

            // ctrl-a
            if (gWasdown[104] && !io.KeysDown[4])
            {
                gWasdown[104] = 0;
                if (table_mode)
                {
                    gArea1 = 0;
                    gArea2 = 27 * MAXROWS - 2;
                }
            }

            // ctrl-c
            if (gWasdown[106] && !io.KeysDown[6])
            {
                gWasdown[106] = 0;
                if (table_mode)
                    copy_area();
            }

            // ctrl-v
            if (gWasdown[125] && !io.KeysDown[25])
            {
                gWasdown[125] = 0;
                if (table_mode)
                    paste_area();
            }

            // del
            if (gWasdown[76] && !io.KeysDown[76])
            {
                gWasdown[76] = 0;
                if (table_mode)
                    del_area();
            }

            // f2
            if (gWasdown[59] && !io.KeysDown[59])
            {
                gWasdown[59] = 0;
                if (table_mode)
                {
                    table_mode = false;
                    gArea1 = -1;
                    focuseditor = true;
                }
                else
                {
                    table_mode = true;
                    focuseditor = false;
                    gArea1 = -1;
                    ImGui::SetWindowFocus("UI"); // need to move focus anywhere else
                }
            }

            // f1
            if (gWasdown[58] && !io.KeysDown[58])
            {
                gWasdown[58] = 0;
                gShowHelpWindow = true;
            }

            // up
            if (gWasdown[82] && !io.KeysDown[82])
            {
                gWasdown[82] = 0;
                if (table_mode)
                {
                    if (io.KeyShift)
                    {
                        if (gArea1 == -1)
                        {
                            gArea1 = gEditorcell;
                        }
                        if (gEditorcell >= 27)
                            gEditorcell -= 27;
                        gArea2 = gEditorcell;
                    }
                    else
                    {
                        gArea1 = -1;
                        if (gEditorcell >= 27)
                            gEditorcell -= 27;                        
                    }
                    scroll_to_editorcell = true;
                }
            }

            // down
            if (gWasdown[81] && !io.KeysDown[81])
            {
                gWasdown[81] = 0;
                if (table_mode)
                {
                    if (io.KeyShift)
                    {
                        if (gArea1 == -1)
                        {
                            gArea1 = gEditorcell;
                        }
                        if (gEditorcell < (MAXROWS-1)*27)
                            gEditorcell += 27;
                        gArea2 = gEditorcell;
                    }
                    else
                    {
                        gArea1 = -1;
                        if (gEditorcell < (MAXROWS-1)*27)
                            gEditorcell += 27;
                    }
                    scroll_to_editorcell = true;
                }
            }

            // left
            if (gWasdown[80] && !io.KeysDown[80])
            {
                gWasdown[80] = 0;
                if (table_mode)
                {
                    if (io.KeyShift)
                    {
                        if (gArea1 == -1)
                        {
                            gArea1 = gEditorcell;
                        }
                        if (gEditorcell % 27)
                            gEditorcell--;
                        gArea2 = gEditorcell;
                    }
                    else
                    {
                        gArea1 = -1;
                        if (gEditorcell % 27)
                            gEditorcell--;
                    }
                    scroll_to_editorcell = true;
                }
            }

            // right
            if (gWasdown[79] && !io.KeysDown[79])
            {
                gWasdown[79] = 0;
                if (table_mode)
                {
                    if (io.KeyShift)
                    {
                        if (gArea1 == -1)
                        {
                            gArea1 = gEditorcell;
                        }
                        if (gEditorcell % 27 != 25)
                            gEditorcell++;
                        gArea2 = gEditorcell;
                    }
                    else
                    {
                        gArea1 = -1;
                        if (gEditorcell % 27 != 25)
                            gEditorcell++;
                    }
                    scroll_to_editorcell = true;
                }
            }

            ImVec2 buttonsize = ImVec2(100 * gConfig.mUIScale, 18 * gConfig.mUIScale);
           
            if (table_mode)
            {
                if (ImGui::Button("Table mode", buttonsize))
                {
                    table_mode = false;
                    gArea1 = -1;
                }
            }
            else
            {
                if (ImGui::Button("Edit mode", buttonsize))
                    table_mode = true;
            }
            ImGui::SameLine();
            if (gMuted)
            {
                if (ImGui::Button("Unmute", buttonsize))
                    gMuted = false;
            }
            else
            {
                if (ImGui::Button("Mute", buttonsize))
                    gMuted = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Warm reset", buttonsize))
                warm_reset();
            ImGui::SameLine();
            if (ImGui::Button("Reload bufs", buttonsize))
            {
                SDL_LockMutex(gAudioMutex);
                nukeResources();
                warm_reset();                
                SDL_UnlockMutex(gAudioMutex);
            }
            ImGui::SameLine();
            if (ImGui::Button(gConfig.mShowUses?"Hide uses":"Show uses", buttonsize))
            {
                gConfig.mShowUses = !gConfig.mShowUses;
            }
            ImGui::SameLine();
            if (ImGui::Button(gConfig.mShowUsedBy ? "Hide used by" : "Show used by", buttonsize))
            {
                gConfig.mShowUsedBy = !gConfig.mShowUsedBy;
            }


            if (openabout)
                ImGui::OpenPopup("about_sassy");

            if (openconfig)
                ImGui::OpenPopup("Preferences");

            about_dialog();

            config_dialog();

            openabout = false;
            openconfig = false;

            if (!io.InputQueueCharacters.empty())
            {
                focuseditor = true;
                table_mode = false;
            }

            static ImWchar focusing_character_kludge = 0;
            if (focuseditor)
            {
                focuseditor = false;
                ImGui::SetKeyboardFocusHere();
                if (!io.InputQueueCharacters.empty())
                {
                    focusing_character_kludge = io.InputQueueCharacters[0];
                }
            }
            else
                if (focusing_character_kludge)
                {
                    io.InputQueueCharacters.push_back(focusing_character_kludge);
                    focusing_character_kludge = 0;
                }
            extern int gSC_EnableColoring;
            extern int gSC_Skip;

            gSC_EnableColoring = !gCelldata[gEditorcell].mText;
            gSC_Skip = 0;

            if (ImGui::InputTextMultiline(
                "##tb", 
                gCelldata[gEditorcell].mRawtext, 
                1024, 
                ImVec2(ImGui::GetWindowWidth(), 16 * 3 * gConfig.mUIScale),
                ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackAlways, // flags
                editor_callback))
            {
                if (ImGui::IsItemFocused())
                {
                    focusing_character_kludge = 0;
                }
                SDL_LockMutex(gAudioMutex);
                poke(gEditorcell);
                rebuild();
                SDL_UnlockMutex(gAudioMutex);
            }
            gSC_EnableColoring = 0;
            if (ImGui::IsItemFocused())
            {
                table_mode = 0;
                gArea1 = -1;
            }

            {
                ImVec2 idpos = ImGui::GetItemRectMin();
                idpos.y += 2 * 16;
                char temp[32];
                sprintf(temp, "%c%d", 'A' + (gEditorcell % 27), (gEditorcell / 27) + 1);
                ImGui::GetWindowDrawList()->AddText(idpos, 0x7fffffff, temp);
            }

            int rowofs = 0;
            ImGui::BeginTabBar("Tabbar");
            if (ImGui::BeginTabItem("  1 -  32"))
            {
                rowofs = 0;
                gEditorcell = (gEditorcell % (32 * 27)) + rowofs * 27;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(" 33 -  64"))
            {
                rowofs = 32;
                gEditorcell = (gEditorcell % (32 * 27)) + rowofs * 27;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(" 65 -  96"))
            {
                rowofs = 64;
                gEditorcell = (gEditorcell % (32 * 27)) + rowofs * 27;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(" 97 - 128"))
            {
                rowofs = 96;
                gEditorcell = (gEditorcell % (32 * 27)) + rowofs * 27;                
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();

            static ImGuiTableFlags flags2 =
                ImGuiTableFlags_Borders |
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;

            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_TableBorderLight, 0x80606060);
            ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, 0x80606060);
            if (ImGui::BeginTable("spreadsheet", 27, flags2, ImVec2(ImGui::GetWindowWidth() - 8 * gConfig.mUIScale, ImGui::GetWindowHeight() -16*8 * gConfig.mUIScale)))
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);

                for (int j = 0; j < 27; j++)
                {
                    float font_size = ImGui::GetFontSize();
                    ImGui::TableNextColumn();
                    if (j == 0)
                        ImGui::TextDisabled("@");
                    else
                    {
                        ImGui::Dummy(ImVec2((56 * gConfig.mUIScale - font_size) / 2, 1));
                        ImGui::SameLine();
                        ImGui::TextDisabled("%c", 'A' + j - 1);
                    }
                }

                for (int i = rowofs + 1; i < 33 + rowofs; i++)
                {
                    for (int j = 0; j < 27; j++)
                    {
                        int cell = j * 27 + i;
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(64 * gConfig.mUIScale);                        
                        ImGui::PushID(cell);
                        if (j == 0)
                            ImGui::TextDisabled("%3d", i);
                        else
                        {
                            int c = (i - 1) * 27 + (j - 1);
                            ImVec4 color = ImVec4(0.4f, 0.4f, 0.6f, 0.5); 
                            if (gCelldata[c].mText)
                                color = ImVec4(0.1f, 0.1f, 0.1f, 0.5);
                            else
                            if (gCellvalue[c] != gCellvalue[c])
                                color = ImVec4(0.5f, 0.1f, 0.1f, 0.5);
                            else
                                if (!gCelldata[c].mDynamic)
                                    color = ImVec4(0.4f, 0.4f, 0.4f, 0.5);
                            if (c == gEditorcell)
                                color = ImVec4(0.2f, 0.2f, 0.75f, 0.5);

                            if (is_area(c) || (c == gEditorcell && table_mode))
                                color = ImVec4(0.2f, 0.2f, 0.75f, 0.5);
                            if (table_mode && scroll_to_editorcell && c == gEditorcell)
                            {
                                ImGui::SetScrollHereX();
                                ImGui::SetScrollHereY();
                            }
                            if (c == gEditorcell && !table_mode)
                                color = ImVec4(0.2f, 0.2f, 0.6f, 0.5);
                            if (gCelldata[c].mHilighttime == gTime)
                                color = ImVec4(
                                    ((gCelldata[c].mHilightcolor >> 16) & 0xff) / 255.0f,
                                    ((gCelldata[c].mHilightcolor >> 8) & 0xff) / 255.0f,
                                    ((gCelldata[c].mHilightcolor >> 0) & 0xff) / 255.0f,
                                    0.5f);
                            //ImVec4(0.1f, 0.3f, 0.1f, 1);                            

                            ImGui::PushStyleColor(ImGuiCol_Button, color);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
                            if (gCelldata[c].mText)
                            {
                                if (gCelldata[c].mRawtext[0] == '\'')
                                {
                                    char temp[64];
                                    snprintf(temp, 64, "%s###%d", gCelldata[c].mRawtext + 1, c);

                                    if (ImGui::Button(temp, ImVec2(64 * gConfig.mUIScale,18 * gConfig.mUIScale)))
                                    {
                                        if (table_mode)
                                        {
                                            if (io.KeyShift)
                                            {
                                                gArea1 = gEditorcell;
                                                gArea2 = c;
                                            }
                                            else
                                            {
                                                gEditorcell = c;                                                
                                                gArea1 = gArea2 = -1;
                                            }
                                        }
                                        else
                                        {
                                            gEditorcell = c;
                                            gArea1 = gArea2 = -1;
                                        }
                                        if (!table_mode)
                                            focuseditor = true;
                                    }
                                    context_menu(c, gArea1, gArea2);                   
                                }
                                else
                                {
                                    char temp[16];
                                    sprintf(temp, "###%d", c);
                                    if (ImGui::Button(temp, ImVec2(64 * gConfig.mUIScale, 18 * gConfig.mUIScale)))
                                    {
                                        if (table_mode)
                                        {
                                            if (io.KeyShift)
                                            {
                                                gArea1 = gEditorcell;
                                                gArea2 = c;
                                            }
                                            else
                                            {
                                                gEditorcell = c;
                                                gArea1 = gArea2 = -1;
                                            }
                                        }
                                        else
                                        {
                                            gEditorcell = c;
                                            gArea1 = gArea2 = -1;
                                        }
                                        if (!table_mode)
                                            focuseditor = true;
                                    }

                                    context_menu(c, gArea1, gArea2);
                                }
                                
                            }
                            else
                            {
                                char temp[64];
                                if (gCelldata[c].mRawtext[0] == '&')
                                {
                                    // note
                                    int v = (int)gCellvalue[c];
                                    if (v < 0) v = 0;
                                    if (v > 127) v = 0;
                                    sprintf(temp, "%s###cell%d", gNotestr[v], c);
                                }
                                else
                                if (gCelldata[c].mRawtext[0] == '#')
                                {
                                    // integer
                                    sprintf(temp, "%d###cell%d", (int)gCellvalue[c], c);
                                }
                                else
                                if (gCelldata[c].mRawtext[0] == '%')
                                {
                                    // bits
                                    int v = (int)gCellvalue[c];
                                    for (int b = 0; b < gConfig.mBitDisplayWidth; b++)
                                    {
                                        temp[b] = (v & (1 << ((gConfig.mBitDisplayWidth - 1) - b))) ? '1' : '0';
                                    }
                                    sprintf(temp + gConfig.mBitDisplayWidth, "###cell%d", c);
                                }
                                else
                                {
                                    // numeric value
                                    sprintf(temp, "%5.3f###cell%d", gCellvalue[c], c);
                                }

                                if (ImGui::Button(temp, ImVec2(64 * gConfig.mUIScale,18 * gConfig.mUIScale)))//ImVec2(-FLT_MIN, 0.0f)))
                                {
                                    if (table_mode)
                                    {
                                        if (io.KeyShift)
                                        {
                                            gArea1 = gEditorcell;
                                            gArea2 = c;
                                        }
                                        else
                                        {
                                            gEditorcell = c;
                                            gArea1 = gArea2 = -1;
                                        }
                                    }
                                    else
                                    {
                                        gEditorcell = c;
                                        gArea1 = gArea2 = -1;
                                    }
                                    if (!table_mode)
                                        focuseditor = true;
                                }
                                context_menu(c, gArea1, gArea2);

                            }
                            ImGui::PopStyleColor(3);
                            if (c == gEditorcell)
                            {
                                editorcell_pos = ImGui::GetItemRectMin();
                                editorcell_pos.x += ImGui::GetItemRectSize().x / 2;
                                editorcell_pos.y += ImGui::GetItemRectSize().y / 2;
                            }
                            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseDragging(0, 0))
                            {
                                ImVec2 dd = ImGui::GetMouseDragDelta(0, 0);
                                if (abs(dd.x) + abs(dd.y) < 5)
                                {
                                    if (io.KeyShift)
                                    {
                                        gArea1 = gEditorcell;
                                    }
                                    else
                                    {
                                        gArea1 = c;
                                        gEditorcell = c;
                                    }
                                }
                                else
                                {
                                    table_mode = true;
                                }
                                gArea2 = c;
                            }
                            if (ImGui::IsItemHovered())
                            {
                                char temp[32];
                                sprintf(temp, "%c%d\n(%d,%d)", 
                                    'A' + (c % 27), (c / 27) + 1, 
                                    (c/27) + 1, (c%27) + 1);
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", temp);
                                ImGui::EndTooltip();
                            }

                        }
                        //ImGui::InputText("##cell", "foo", IM_ARRAYSIZE(text_bufs[cell]));
                        ImGui::PopID();
                    }
                }
                ImGui::PopStyleColor();
                ImGui::EndTable();
                grid_min = ImGui::GetItemRectMin();
                grid_max = ImGui::GetItemRectMax();
                grid_max.x -= 12 * gConfig.mUIScale;
                grid_max.y -= 12 * gConfig.mUIScale;

            }
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(2);

            ImGui::Text("JIT code: %d bytes", gAsmcode.getSize());

            if (gConfig.mShowCPU)
            {
                ImGui::SameLine();
                ImGui::Text(" | CPU use");
                ImGui::SameLine();

                if (gCPU < 10)
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 0.5, 0, 1));
                else
                    if (gCPU < 30)
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5, 0.5, 0, 1));
                    else
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1, 0, 0, 1));
                ImGui::ProgressBar(gCPU * 0.01f, ImVec2(64 * gConfig.mUIScale, 18 * gConfig.mUIScale));
                ImGui::PopStyleColor(1);
            }
            if (gConfig.mShowCPUPeak)
            {
                ImGui::SameLine();
                ImGui::Text("| CPU Peak:");
                ImGui::SameLine();
                if (gCPU_peak < 10)
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 0.5, 0, 1));
                else
                    if (gCPU_peak < 30)
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5, 0.5, 0, 1));
                    else
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1, 0, 0, 1));

                ImGui::ProgressBar(gCPU_peak * 0.01f, ImVec2(64 * gConfig.mUIScale, 18 * gConfig.mUIScale));

                ImGui::PopStyleColor(1);
            }
            if (gConfig.mShowCPUAvg)
            {
                ImGui::SameLine();
                ImGui::Text(" | CPU avg");
                ImGui::SameLine();

                if (gCPU_avg_sum < 10 * AVG_SAMPLES)
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 0.5, 0, 1));
                else
                    if (gCPU_avg_sum < 30 * AVG_SAMPLES)
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5, 0.5, 0, 1));
                    else
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1, 0, 0, 1));
                ImGui::ProgressBar(gCPU_avg_sum / (AVG_SAMPLES * 100.0f), ImVec2(64 * gConfig.mUIScale, 18 * gConfig.mUIScale));
                ImGui::PopStyleColor(1);
            }
            if (gConfig.mShowMidiMessages)
            {
                ImGui::SameLine();
                ImGui::Text("| MIDI: %s", gMidimessage);
            }

            ImGui::End();
        }

        uibar(mainwindoriginalsize);

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());        
        SDL_Delay(10);
        SDL_GL_SwapWindow(window);

        if (first_cycle)
        {
            first_cycle = false;
            openabout = true;
        }
    }
    printf("Shutting down..\n");
    
    shutdown = true;
    // Cleanup
    
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (gAudioDeviceID)
    {
        SDL_CloseAudioDevice(gAudioDeviceID);
    }
    if (gCaptureDeviceID)
    {
        SDL_CloseAudioDevice(gCaptureDeviceID);
    }
    gRtmidi_in->closePort();
    gRtmidi_out->closePort();

    if (gResourceLoaderRunning)
    {
        printf("Waiting for resource loader thread..\n");
        int watchdog = 10000;
        while (watchdog && gResourceLoaderRunning)
        {
            SDL_Delay(1);
            watchdog--;
        }
        if (gResourceLoaderRunning)
        {
            printf("Giving up waiting, continuing shutdown. We may crash.\n");
        }
    }

    SDL_Delay(500);

    delete gRtmidi_in;
    delete gRtmidi_out;
    
    SDL_DestroyMutex(gAudioMutex);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
