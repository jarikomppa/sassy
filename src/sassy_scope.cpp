#include "sassy.h"

static const unsigned int probecolors[4] =
{
    0xffc0c0c0,
    0xffa0a0ff,
    0xffffa0a0,
    0xff30d0d0
};

static const float timescalesteps[5] =
{
    0.0001f,
    0.001f,
    0.01f,
    0.1f,
    1.0f,
};

static const char* timescaletext[5] =
{
    "0.1ms",
    "1ms",
    "10ms",
    "100ms",
    "1000ms"
};

static const float scalesteps[9] =
{
    1.0f / 16.0f,
    1.0f / 8.0f,
    1.0f / 4.0f,
    1.0f / 2.0f,
    1.0f,
    2.0f,
    4.0f,
    8.0f,
    16.0f,
};

static const char* scaletexts[9] = {
    "x1/16",
    "x1/8",
    "x1/4",
    "x1/2",
    "x1",
    "x2",
    "x4",
    "x8",
    "x16"
};

int gFFTAverage = 1;

void scope_grid()
{
    ImVec2 p = ImGui::GetItemRectMin();

    // zero
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221) * gConfig.mUIScale), 0xff000000, 3.0f);
    // 1.0
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 - 110) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 - 110) * gConfig.mUIScale), 0xff000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 + 110) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 + 110) * gConfig.mUIScale), 0xff000000);
    // 0.5
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 - 55) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 - 55) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 + 55) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 + 55) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 - 165) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 - 165) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + (221 + 165) * gConfig.mUIScale), ImVec2(p.x + (442) * gConfig.mUIScale, p.y + (221 + 165) * gConfig.mUIScale), 0x3f000000);

    // zero
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0xff000000, 3.0f);
    // 1.0
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 - 110) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 - 110) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0xff000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 + 110) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 + 110) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0xff000000);
    // 0.5
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 - 55) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 - 55) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 + 55) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 + 55) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 - 165) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 - 165) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0x3f000000);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + (221 + 165) * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + (221 + 165) * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0x3f000000);
}


int scope_sync(int index)
{
    int samples = (int)(gSamplerate * gScope->mTimeScale);

    ImVec2 p = ImGui::GetItemRectMin();
    int cycle = gSamplerate * 10;

    int ofs = samples;

    if (gScope->mMode == 0)
    {
        // calculate sync
        if (gScope->mSyncMode == 0)
        {
            float* graphdata = gScope->mCh[gScope->mSyncChannel].mData;
            int over = ofs;
            while (over < (cycle - ofs) && graphdata[(index - over + cycle) % cycle] < 0) over++;
            int under = over;
            while (under < (cycle - ofs) && graphdata[(index - under + cycle) % cycle] > 0) under++;
            ofs = under;
        }
        else
            if (gScope->mSyncMode == 1)
            {
                float* graphdata = gScope->mCh[gScope->mSyncChannel].mData;
                int under = ofs;
                while (under < (cycle - ofs) && graphdata[(index - under + cycle) % cycle] > 0) under++;
                int over = under;
                while (over < (cycle - ofs) && graphdata[(index - over + cycle) % cycle] < 0) over++;
                ofs = over;
            }
        // default: ofs = samples

    }
    else
    {
        // pause mode, scroll bar is active
        ofs = -(int)(gScope->mScroll * gSamplerate);
        if (ofs < samples)
            ofs = samples;
        if (ofs > gSamplerate * 10 - samples)
            ofs = gSamplerate * 10 - samples;
    }
    gScope->mScroll = -((float)ofs / gSamplerate);

    return ofs;
}

void scope_plot(int index)
{
    ImVec2 p = ImGui::GetItemRectMin();
    int cycle = gSamplerate * 10;
    /*
    Okay, max scale is 1 second, so..
    */
    int samples = (int)(gSamplerate * gScope->mTimeScale);

    scope_grid();

    int ofs = scope_sync(index);

    if (gScope->mDisplay == 2)
    {
        for (int i = 0; i < 16384; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (gScope->mCh[j].mEnabled)
                {
                    float* graphdata = gScope->mCh[j].mData;
                    float y = graphdata[(index - ofs + i * samples / 16384 + cycle) % cycle];
                    float x = graphdata[(index - ofs + i * samples / 16384 + cycle - 1) % cycle];
                    x = x * gScope->mCh[j].mScale;
                    y = y * gScope->mCh[j].mScale - gScope->mCh[j].mOffset;
                    ImGui::GetWindowDrawList()->AddCircleFilled(
                        ImVec2(p.x + (221 + x * 110) * gConfig.mUIScale, p.y + (221 + y * 110) * gConfig.mUIScale),
                        1,
                        (probecolors[j] & 0xffffff) | 0x3f000000);
                }
            }
        }
    }
    else
    {
        for (int i = 0; i < 32768; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                if (gScope->mCh[j*2].mEnabled)
                {
                    float* graphdata = gScope->mCh[j * 2].mData;
                    float x = graphdata[(index - ofs + i * samples / 32768 + cycle) % cycle];
                    graphdata = gScope->mCh[j * 2 + 1].mData;
                    float y = graphdata[(index - ofs + i * samples / 32768 + cycle) % cycle];
                    x = x * gScope->mCh[j * 2].mScale - gScope->mCh[j * 2].mOffset;
                    y = y * gScope->mCh[j * 2 + 1].mScale - gScope->mCh[j * 2 + 1].mOffset;
                    ImGui::GetWindowDrawList()->AddCircleFilled(
                        ImVec2(p.x + (221 + x * 110) * gConfig.mUIScale, p.y + (221 + y * 110) * gConfig.mUIScale),
                        1,
                        (probecolors[j*2] & 0xffffff) | 0x3f000000);
                }
            }
        }
    }

}

void scope_time(int index)
{
    ImVec2 p = ImGui::GetItemRectMin();
    int cycle = gSamplerate * 10;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    /*
    Okay, max scale is 1 second, so..
    */
    int samples = (int)(gSamplerate * gScope->mTimeScale);

    scope_grid();

    int ofs = scope_sync(index);

    if (samples >= 441)
    {
        for (int j = 0; j < 4; j++)
        {
            if (gScope->mCh[j].mEnabled)
            {
                float* graphdata = gScope->mCh[j].mData;
                ImVec2 vert[441];
                for (int i = 0; i < 441; i++)
                {
                    float v0 = -graphdata[(index - ofs + i * samples / 442 + cycle) % cycle];
                    float v1 = -graphdata[(index - ofs + (i + 1) * samples / 442 + cycle) % cycle];
                    v0 = v0 * gScope->mCh[j].mScale - gScope->mCh[j].mOffset;
                    v1 = v1 * gScope->mCh[j].mScale - gScope->mCh[j].mOffset;
                    vert[i].x = p.x + i * gConfig.mUIScale;
                    vert[i].y = p.y + (221 + v0 * 110) * gConfig.mUIScale;
                }
                float v0 = p.y + (221 + (-gScope->mCh[j].mOffset) * 110) * gConfig.mUIScale;
                dl->Flags = 0;
                for (int i = 0; i < 440; i++)
                {
                    ImVec2 quad[4];
                    quad[0] = ImVec2(vert[i].x, v0);
                    quad[1] = ImVec2(vert[i].x, vert[i].y);
                    quad[2] = ImVec2(vert[i + 1].x, vert[i + 1].y);
                    quad[3] = ImVec2(vert[i + 1].x, v0);
                    dl->AddConvexPolyFilled(quad, 4, (probecolors[j] & 0xffffff) | 0x3f000000 );

                }
                
                if (gScope->mTimeScale < 0.1)
                {
                    dl->Flags = ImDrawListFlags_AntiAliasedLines;
                    dl->AddPolyline(vert, 441, probecolors[j], false, 2 * gConfig.mUIScale);
                }
                else
                {
                    dl->Flags = ImDrawListFlags_AntiAliasedLines;
                    dl->AddPolyline(vert, 441, probecolors[j], false, 1);

                }
            }
        }
    }
    else
    {
        // less than 1 sample per pixel
        for (int j = 0; j < 4; j++)
        {
            if (gScope->mCh[j].mEnabled)
            {
                float* graphdata = gScope->mCh[j].mData;
                for (int i = 0; i < samples; i++)
                {
                    float v0 = -graphdata[(index - ofs + i + cycle) % cycle];
                    float v1 = 0;
                    v0 = v0 * gScope->mCh[j].mScale - gScope->mCh[j].mOffset;
                    v1 = v1 * gScope->mCh[j].mScale - gScope->mCh[j].mOffset;
                    float x0 = p.x + (i * 442 / samples) * gConfig.mUIScale;
                    ImGui::GetWindowDrawList()->AddCircleFilled(
                        ImVec2(x0, p.y + (221 + v0 * 110) * gConfig.mUIScale),
                        4 * gConfig.mUIScale,
                        probecolors[j]);
                    ImGui::GetWindowDrawList()->AddLine(
                        ImVec2(x0, p.y + (221 + v0 * 110) * gConfig.mUIScale),
                        ImVec2(x0, p.y + (221 + v1 * 110) * gConfig.mUIScale),
                        probecolors[j]);
                }
            }
        }
    }

    ImGui::GetWindowDrawList()->AddText(p, 0xffc0c0c0, timescaletext[gScope->mTimeScaleSlider + 2]);
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + (221 - 110 - 7) * gConfig.mUIScale), 0xffc0c0c0, "+1");
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x, p.y + (221 + 110 - 7) * gConfig.mUIScale), 0xffc0c0c0, "-1");

    ImVec2 mp = ImGui::GetMousePos();
    mp.x -= p.x;
    mp.y -= p.y;
    if (mp.x > 0 && mp.x < 442 * gConfig.mUIScale &&
        mp.y > 0 && mp.y < 442 * gConfig.mUIScale)
    {
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(p.x + mp.x, p.y),
            ImVec2(p.x + mp.x, p.y + 442 * gConfig.mUIScale),
            0xff00ff00, 1 * gConfig.mUIScale);
        if (gScope->mCh[0].mEnabled || gScope->mCh[1].mEnabled || gScope->mCh[2].mEnabled || gScope->mCh[3].mEnabled)
        {
            ImGui::BeginTooltip();
            if (gScope->mCh[0].mEnabled) ImGui::Text("Ch 0: %3.3f", gScope->mCh[0].mData[(index - ofs + ((int)mp.x / (int)gConfig.mUIScale) * samples / 442 + cycle) % cycle]);
            if (gScope->mCh[1].mEnabled) ImGui::Text("Ch 1: %3.3f", gScope->mCh[1].mData[(index - ofs + ((int)mp.x / (int)gConfig.mUIScale) * samples / 442 + cycle) % cycle]);
            if (gScope->mCh[2].mEnabled) ImGui::Text("Ch 2: %3.3f", gScope->mCh[2].mData[(index - ofs + ((int)mp.x / (int)gConfig.mUIScale) * samples / 442 + cycle) % cycle]);
            if (gScope->mCh[3].mEnabled) ImGui::Text("Ch 3: %3.3f", gScope->mCh[3].mData[(index - ofs + ((int)mp.x / (int)gConfig.mUIScale) * samples / 442 + cycle) % cycle]);
            ImGui::EndTooltip();
        }
    }
}


void vertline(float x, float w)
{
    ImVec2 p = ImGui::GetItemRectMin();
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x + x * gConfig.mUIScale, p.y * gConfig.mUIScale), ImVec2(p.x + x * gConfig.mUIScale, p.y + (442) * gConfig.mUIScale), 0xff000000, w);
}

void scope_freq(int index)
{
    ImVec2 p = ImGui::GetItemRectMin();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    int cycle = gSamplerate * 10;
    /*
    Okay, max scale is 1 second, so..
    */
    int samples = (int)(gSamplerate * gScope->mTimeScale);

    // what's the biggest PoT < samples?
    // 192000 takes 18 bits to encode.
    // Fill 32 bits:
    int pot = samples | (samples >> 16);
    pot = pot | (pot >> 8);
    pot = pot | (pot >> 4);
    pot = pot | (pot >> 2);
    pot = pot | (pot >> 1);
    // Shift down and add one to round it up
    pot = (pot >> 1) + 1;

    if (pot < 16) pot = 16;
    if (pot > 65536) pot = 65536;

    gScope->mPot = pot;

    ffft::FFTReal<float>* fft = NULL;
    switch (pot)
    {
    case 16: fft = &gFft_object_16; break;
    case 32: fft = &gFft_object_32; break;
    case 64: fft = &gFft_object_64; break;
    case 128: fft = &gFft_object_128; break;
    case 256: fft = &gFft_object_256; break;
    case 512: fft = &gFft_object_512; break;
    case 1024: fft = &gFft_object_1024; break;
    case 2048: fft = &gFft_object_2048; break;
    case 4096: fft = &gFft_object_4096; break;
    case 8192: fft = &gFft_object_8192; break;
    case 16384: fft = &gFft_object_16384; break;
    case 32768: fft = &gFft_object_32768; break;
    case 65536: fft = &gFft_object_65536; break;
    }
    if (!fft) return;

    int ofs = scope_sync(index);
    float freqbin = gSamplerate / (float)(pot / 2);
    float freqbins[441];
    float zoom = 1.0f / (1 << gScope->mFFTZoom);

    for (int i = 0; i < 10; i++)
    {     
        vertline(sqrt(100 / freqbin * i / (pot / 4)) / zoom * 441.0f, 1);
        vertline(sqrt(1000 / freqbin * i / (pot / 4)) / zoom * 441.0f, 1);
        vertline(sqrt(10000 / freqbin * i / (pot / 4)) / zoom * 441.0f, 1);
    }

    for (int j = 0; j < 4; j++)
    {
        if (gScope->mCh[j].mEnabled)
        {


            memset(gScope->ffta, 0, sizeof(float) * 65536 * 2);
            for (int k = 0; k < gFFTAverage; k++)
            {
                float* graphdata = gScope->mCh[j].mData;

                for (int i = 0; i < pot; i++)
                {
                    gScope->fft1[i * 2] = graphdata[(index - ofs + i + cycle - k) % cycle];
                    gScope->fft1[i * 2 + 1] = 0;
                }

                fft->do_fft(gScope->fft2, gScope->fft1);

                for (int i = 0; i < pot / 4; i++)
                    gScope->ffta[i] += (1.0f / gFFTAverage) * sqrt(gScope->fft2[i * 2 + 0] * gScope->fft2[i * 2 + 0] + gScope->fft2[i * 2 + 1] * gScope->fft2[i * 2 + 1]);
            }

            ImVec2 vert[441];

            for (int i = 0; i < 441; i++)
            {
                float ppos = powf(zoom * i / 441.0f, 2.0f) * pot / 4;
                freqbins[i] = ppos * freqbin;
                
                float f = ppos - (int)ppos;
                float a = i ? gScope->ffta[(int)ppos - 1] : 0;
                float b = gScope->ffta[(int)ppos];
                float c = i < 441 ? gScope->ffta[(int)ppos + 1] : 0;
                float d = i < 440 ? gScope->ffta[(int)ppos + 2] : 0;

                float v0 = (float)EvalFunc::catmullrom(f, a, b, c, d);
                
                v0 = v0 * gScope->mCh[j].mScale + gScope->mCh[j].mOffset * 50;
                vert[i] = ImVec2(p.x + i * gConfig.mUIScale, 
                                 p.y + (441 - v0 * 4) * gConfig.mUIScale);
            }
            float v0 = p.y + (441 - gScope->mCh[j].mOffset * 50 * 4) * gConfig.mUIScale;
            dl->Flags = 0;
            for (int i = 0; i < 440; i++)
            {
                ImVec2 quad[4];
                quad[0] = ImVec2(vert[i].x, v0);
                quad[1] = ImVec2(vert[i].x, vert[i].y);
                quad[2] = ImVec2(vert[i + 1].x, vert[i + 1].y);
                quad[3] = ImVec2(vert[i + 1].x, v0);
                dl->AddConvexPolyFilled(quad, 4, (probecolors[j] & 0xffffff) | 0x3f000000);

            }

            dl->Flags = ImDrawListFlags_AntiAliasedLines;
            dl->AddPolyline(vert, 441, probecolors[j], false, 1);

        }
    }

    if (!ImGui::IsPopupOpen("Freq Context",ImGuiPopupFlags_AnyPopupId))
    if (gScope->mCh[0].mEnabled || gScope->mCh[1].mEnabled || gScope->mCh[2].mEnabled || gScope->mCh[3].mEnabled)
    {
        ImVec2 mp = ImGui::GetMousePos();
        mp.x -= p.x;
        mp.y -= p.y;
        if (mp.x > 0 && mp.x < 442 * gConfig.mUIScale &&
            mp.y > 0 && mp.y < 442 * gConfig.mUIScale)
        {
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(p.x + mp.x, p.y),
                ImVec2(p.x + mp.x, p.y + 442 * gConfig.mUIScale),
                0xff00ff00, 1 * gConfig.mUIScale);
            ImGui::BeginTooltip();
            int note = (int)(12 * log(32 * POW_2_3_4TH * (freqbins[(int)mp.x] / 440)) / log(2));            
            if (note < 0 || note > 127) note = -1;
            ImGui::Text("%3.3fHz%s%s", freqbins[(int)mp.x], note==-1?"":"\n", note==-1?"":gNotestr[note]);
            ImGui::EndTooltip();
        }
    }
}

int groups(double h)
{
    int count = 0;
    for (int i = 1; i < gScope->mPot / 4; i++)
    {
        if (gScope->fft1[i - 1] < h && gScope->fft1[i] > h)
            count++;
    }
    return count;
}

void detect_fundamentals()
{
    // gScope->fft1[1..pot/4] has our mags
    double maxmag = 0;
    for (int i = 0; i < gScope->mPot / 4; i++)
        if (maxmag < gScope->fft1[i]) 
            maxmag = gScope->fft1[i];

    double minmag = 0;
    int count = 0;
    int iters = 0;
    double h = (minmag + maxmag) / 2;
    double step = h / 2;
    while (iters < 100 && count != 16)
    {
        count = groups(h);
        if (count < 16)
        { 
            h -= step;
        }
        else
        {
            h += step;
        }
        step /= 2;
        iters++;
    }
    char temp[1024];
    int ofs = 0;
    temp[0] = 0;
    float freqbin = gSamplerate / (float)(gScope->mPot / 2);

    int startbin = 0;
    for (int i = 2; i < gScope->mPot / 4; i++)
    {
        if (gScope->fft1[i - 1] < h && gScope->fft1[i] > h)
        {
            startbin = i;
        }
        if (gScope->fft1[i - 1] > h && gScope->fft1[i] < h)
        {
            double sum = 0;
            double magsum = 0;
            for (int j = startbin; j < i; j++)
            {
                sum += gScope->fft1[j];
                magsum += gScope->fft1[j] * j * freqbin;
            }
            if (sum != 0)
            {
                magsum /= sum;
                sum /= i - startbin;
                sum /= maxmag / 2; // normalize
                ofs += sprintf(temp + ofs, "%3.3f\t%3.3f\n", magsum, sum);
            }
        }
    }


    for (int i = 0; i < count; i++)
    {
    }
    ImGui::SetClipboardText(temp);
}


void do_show_scope_window()
{
    // Data is updated live, so let's take local copies of critical stuff.
    int index = gScope->mIndex;


    ImGui::Begin("Scope", &gShowScopeWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::BeginChild("Channel options", ImVec2((4 * 25)*gConfig.mUIScale, (2 * 202 + 32) * gConfig.mUIScale));
    ImGui::Checkbox("###ea", &gScope->mCh[0].mEnabled); ImGui::SameLine();
    ImGui::Checkbox("###eb", &gScope->mCh[1].mEnabled); ImGui::SameLine();
    ImGui::Checkbox("###ec", &gScope->mCh[2].mEnabled); ImGui::SameLine();
    ImGui::Checkbox("###ed", &gScope->mCh[3].mEnabled);

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[0]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[0]);
    if (ImGui::VSliderInt("###0a", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[0].mScaleSlider, -4, 4, ""))
    {
        gScope->mCh[0].mScale = scalesteps[gScope->mCh[0].mScaleSlider + 4];
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(scaletexts[gScope->mCh[0].mScaleSlider + 4]);
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);  ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[1]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[1]);
    if (ImGui::VSliderInt("###1a", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[1].mScaleSlider, -4, 4, ""))
    {
        gScope->mCh[1].mScale = scalesteps[gScope->mCh[1].mScaleSlider + 4];
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(scaletexts[gScope->mCh[1].mScaleSlider + 4]);
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);  ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[2]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[2]);
    if (ImGui::VSliderInt("###2a", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[2].mScaleSlider, -4, 4, ""))
    {
        gScope->mCh[2].mScale = scalesteps[gScope->mCh[2].mScaleSlider + 4];
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(scaletexts[gScope->mCh[2].mScaleSlider + 4]);
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);  ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[3]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[3]);
    if (ImGui::VSliderInt("###3a", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[3].mScaleSlider, -4, 4, ""))
    {
        gScope->mCh[3].mScale = scalesteps[gScope->mCh[3].mScaleSlider + 4];
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(scaletexts[gScope->mCh[3].mScaleSlider + 4]);
        ImGui::EndTooltip();
    }
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[0]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[0]); ImGui::VSliderFloat("###0b", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[0].mOffset, -2, 2, ""); ImGui::PopStyleColor(2);   ImGui::SameLine();
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%3.3f", gScope->mCh[0].mOffset);
        ImGui::EndTooltip();
    }
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[1]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[1]); ImGui::VSliderFloat("###1b", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[1].mOffset, -2, 2, ""); ImGui::PopStyleColor(2);   ImGui::SameLine();
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%3.3f", gScope->mCh[1].mOffset);
        ImGui::EndTooltip();
    }
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[2]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[2]); ImGui::VSliderFloat("###2b", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[2].mOffset, -2, 2, ""); ImGui::PopStyleColor(2);   ImGui::SameLine();
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%3.3f", gScope->mCh[2].mOffset);
        ImGui::EndTooltip();
    }
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, probecolors[3]); ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, probecolors[3]); ImGui::VSliderFloat("###3b", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mCh[3].mOffset, -2, 2, ""); ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%3.3f", gScope->mCh[3].mOffset);
        ImGui::EndTooltip();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
    ImGui::BeginChild("Scope and scroll", ImVec2(442 * gConfig.mUIScale, (442 + 24)* gConfig.mUIScale));
    ImGui::BeginChild("Scope proper", ImVec2(442 * gConfig.mUIScale, 442 * gConfig.mUIScale));

    if (gScope->mDisplay == 0)
        scope_time(index);
    if (gScope->mDisplay == 1)
        scope_freq(index);
    if (gScope->mDisplay == 2 || gScope->mDisplay == 3)
        scope_plot(index);

    ImGui::EndChild();
    ImGui::PopStyleColor(1);
    if (gScope->mDisplay == 1)
    {
        if (ImGui::BeginPopupContextItem("Freq Context"))
        {
            if (ImGui::BeginMenu("Experimental.."))
            {
                if (ImGui::MenuItem("Detect and copy fundamental frequencies"))
                {
                    detect_fundamentals();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Averaging.."))
            {
                if (ImGui::MenuItem("1x"))
                    gFFTAverage = 1;
                if (ImGui::MenuItem("4x"))
                    gFFTAverage = 4;
                if (ImGui::MenuItem("16x"))
                    gFFTAverage = 16;
                if (ImGui::MenuItem("64x"))
                    gFFTAverage = 64;
                if (ImGui::MenuItem("256x"))
                    gFFTAverage = 256;
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }
    //context_menu(1, 1, 1);

    if (gScope->mMode)
    {
        ImGui::SetNextItemWidth(442 * gConfig.mUIScale);
        ImGui::SliderFloat("###scroll", &gScope->mScroll, -10.0f, 0.0f, "%.3f s");
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xff3f3f3f);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0xff3f3f3f);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0xff3f3f3f);
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, 0xff7f7f7f);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, 0xff7f7f7f);
        ImGui::SetNextItemWidth(442 * gConfig.mUIScale);
        float x = gScope->mScroll;
        ImGui::SliderFloat("###scroll", &x, -10.0f, 0.0f, "%.3f s");
        ImGui::PopStyleColor(5);
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("Scope options", ImVec2(4 * 20 * gConfig.mUIScale, 2 * 221 * gConfig.mUIScale));
    if (ImGui::VSliderInt("###0a", ImVec2(19 * gConfig.mUIScale, 200 * gConfig.mUIScale), &gScope->mTimeScaleSlider, -2, 2, ""))
    {
        gScope->mTimeScale = timescalesteps[gScope->mTimeScaleSlider + 2];
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", timescaletext[gScope->mTimeScaleSlider + 2]);
        ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ImGui::BeginChild("moderadio", ImVec2(100 * gConfig.mUIScale, 200 * gConfig.mUIScale));
    if (ImGui::RadioButton("Time", gScope->mDisplay == 0)) gScope->mDisplay = 0;
    if (ImGui::RadioButton("Freq", gScope->mDisplay == 1)) gScope->mDisplay = 1;
    if (ImGui::RadioButton("X,X'", gScope->mDisplay == 2)) gScope->mDisplay = 2;
    if (ImGui::RadioButton("X,Y", gScope->mDisplay == 3)) gScope->mDisplay = 3;
    ImGui::Separator();
    ImGui::Text("fft");
    if (ImGui::RadioButton("1x", gScope->mFFTZoom == 0)) gScope->mFFTZoom = 0;
    if (ImGui::RadioButton("2x", gScope->mFFTZoom == 1)) gScope->mFFTZoom = 1;
    if (ImGui::RadioButton("4x", gScope->mFFTZoom == 2)) gScope->mFFTZoom = 2;
    if (ImGui::RadioButton("8x", gScope->mFFTZoom == 3)) gScope->mFFTZoom = 3;
    ImGui::EndChild();
    char temp[64];
    sprintf(temp, "Sync ch %d###sc", gScope->mSyncChannel);
    if (ImGui::Button(temp, ImVec2(80 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        gScope->mSyncChannel = (gScope->mSyncChannel + 1) % 4;
    const char* syncmodes[3] = { "^", "v", "off" };
    sprintf(temp, "Sync %s###sm", syncmodes[gScope->mSyncMode]);
    if (ImGui::Button(temp, ImVec2(80 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        gScope->mSyncMode = (gScope->mSyncMode + 1) % 3;


    if (gScope->mMode == 0)
    {
        if (ImGui::Button("Pause", ImVec2(80 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
            gScope->mMode = 1;
        ImGui::Text("Nudge (ms)");
        ImGui::PushStyleColor(ImGuiCol_Button, 0xff3f3f3f);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xff3f3f3f);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xff3f3f3f);
        ImGui::Button("-0.1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::SameLine();
        ImGui::Button("+0.1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::Button("-1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::SameLine();
        ImGui::Button("+1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::Button("-10", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::SameLine();
        ImGui::Button("+10", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::Button("-100", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::SameLine();
        ImGui::Button("+100", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale));
        ImGui::PopStyleColor(3);
    }
    else
    {
        if (ImGui::Button("Capture", ImVec2(80 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
            gScope->mMode = 0;
        ImGui::Text("Nudge (ms)");
        if (ImGui::Button("-0.1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll -= 0.0001f;
        }
        ImGui::SameLine();
        if (ImGui::Button("+0.1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll += 0.0001f;
        }
        if (ImGui::Button("-1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll -= 0.001f;
        }
        ImGui::SameLine();
        if (ImGui::Button("+1", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll += 0.001f;
        }
        if (ImGui::Button("-10", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll -= 0.01f;
        }
        ImGui::SameLine();
        if (ImGui::Button("+10", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll += 0.01f;
        }
        if (ImGui::Button("-100", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll -= 0.1f;
        }
        ImGui::SameLine();
        if (ImGui::Button("+100", ImVec2(38 * gConfig.mUIScale, 20 * gConfig.mUIScale)))
        {
            gScope->mScroll += 0.1f;
        }
    }
    ImGui::EndChild();

    ImGui::End();

}
