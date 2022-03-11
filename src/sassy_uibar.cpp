#include "sassy.h"

void myPlotLines(const char* label, const float* values, int values_count, float scale_min, float scale_max, ImVec2 graph_size)
{
    ImGui::BeginChild(label, graph_size);
    ImVec2 p = ImGui::GetItemRectMin();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 pts[512]; // 512 points ought to be enough for everybody
    float htscale = graph_size.y / (scale_max - scale_min);
    for (int i = 0; i < values_count; i++)
    {
        pts[i] = ImVec2(p.x + (i * graph_size.x / values_count),
                        p.y + graph_size.y - (values[i] - scale_min) * htscale);
    }
    dl->Flags = 0;
    for (int i = 0; i < values_count - 1; i++)
    {
        ImVec2 quad[4];
        quad[0] = ImVec2(pts[i].x, p.y + graph_size.y - htscale * -scale_min);
        quad[1] = ImVec2(pts[i].x, pts[i].y);
        quad[2] = ImVec2(pts[i+1].x, pts[i+1].y);
        quad[3] = ImVec2(pts[i + 1].x, p.y + graph_size.y - htscale * -scale_min);
        dl->AddConvexPolyFilled(quad, 4, 0x3fffcccc);

    }
    //dl->Flags = 0;
    //dl->AddPolyline(pts, values_count, 0xafffcccc, false, 1);
    dl->Flags = ImDrawListFlags_AntiAliasedLines;
    dl->AddPolyline(pts, values_count, 0xafffcccc, false, 2*gConfig.mUIScale);
    ImGui::EndChild();
}

void drawRoundRect(ImDrawList *dl, ImVec2 center, float size, unsigned int color)
{
    ImVec2 tl = ImVec2(center.x - size, center.y - size);
    ImVec2 br = ImVec2(center.x + size, center.y + size);
    dl->AddRectFilled(tl, br, color, 4 * gConfig.mUIScale);
}

bool myButton(const char* name)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton(name, ImVec2(24 * gConfig.mUIScale, 20 * gConfig.mUIScale));
    ImVec2 p = ImGui::GetItemRectMin();

    p.x += (12 + 1) * gConfig.mUIScale;
    p.y += (12 + 1) * gConfig.mUIScale;
    drawRoundRect(dl, p, 10 * gConfig.mUIScale, 0xff000000);

    p.x -= ImGui::IsItemActive() ? 1 : 2;
    p.y -= ImGui::IsItemActive() ? 1 : 2;    

    drawRoundRect(dl, p, 10 * gConfig.mUIScale, ImGui::IsItemHovered() ? 0x9fffcccc : 0x7fffcccc);
    return ImGui::IsItemActive();
}

bool myToggle(const char* name, bool state)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton(name, ImVec2(24 * gConfig.mUIScale, 26 * gConfig.mUIScale));
    ImVec2 p = ImGui::GetItemRectMin();
    p.x += 2 * gConfig.mUIScale;
    dl->AddLine(
        ImVec2(p.x + 9 * gConfig.mUIScale, p.y - 6 * gConfig.mUIScale), 
        ImVec2(p.x + 9 * gConfig.mUIScale, p.y + 26 * gConfig.mUIScale), 
        0x3fcccccc, 6 * gConfig.mUIScale);

    dl->AddRectFilled(
        ImVec2(p.x + 2 * gConfig.mUIScale, p.y + (2 - 2 + (state ? 0 : 14)) * gConfig.mUIScale),
        ImVec2(p.x + (2 + 20) * gConfig.mUIScale, p.y + (2 + 10 + (state ? 0 : 14)) * gConfig.mUIScale),
        0x7f000000,
        4 * gConfig.mUIScale);

    dl->AddRectFilled(
        ImVec2(p.x, p.y + (-2 + (state ? 0 : 14)) * gConfig.mUIScale),
        ImVec2(p.x + 20 * gConfig.mUIScale, p.y + (10 + (state ? 0 : 14)) * gConfig.mUIScale),
        (state ? 0xffcccc : 0xccccff) | (ImGui::IsItemHovered() ? 0xcf000000 : 0x9f000000),
        4 * gConfig.mUIScale);
    return ImGui::IsItemActivated();
}

bool mySliderFloat(const char* name, const ImVec2& size, float* v, float v_min, float v_max)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton(name, size);
    ImVec2 p = ImGui::GetItemRectMin();    
    dl->AddLine(
        ImVec2(p.x + 9 * gConfig.mUIScale, p.y),
        ImVec2(p.x + 9 * gConfig.mUIScale, p.y + size.y),
        0x3fcccccc, 6 * gConfig.mUIScale);

    if (v_max == v_min) v_max += 0.1f;
    float pos = 1 - ((*v - v_min) / (v_max - v_min));
    float barht = 12 * gConfig.mUIScale;

    dl->AddRectFilled(
        ImVec2(p.x + 2 * gConfig.mUIScale, p.y + 2 * gConfig.mUIScale+ (size.y - barht) * pos),
        ImVec2(p.x + (2 + 20) * gConfig.mUIScale, p.y + 2 * gConfig.mUIScale + (size.y - barht) * pos + barht),
        0x7f000000,
        4 * gConfig.mUIScale);

    dl->AddRectFilled(
        ImVec2(p.x + 0 * gConfig.mUIScale, p.y + (size.y - barht) * pos),
        ImVec2(p.x + (0 + 20) * gConfig.mUIScale, p.y + (size.y - barht) * pos + barht),
        0xffcccc | (ImGui::IsItemHovered() ? 0xcf000000 : 0x9f000000),
        4 * gConfig.mUIScale);

    if (ImGui::IsItemActive())
    {
        ImVec2 mp = ImGui::GetMousePos();
        float linpos = (1 - ((mp.y - p.y) / size.y));
        if (linpos < 0) linpos = 0;
        if (linpos > 1) linpos = 1;
        *v = linpos * (v_max - v_min) + v_min;
    }

    return ImGui::IsItemActive();
}


bool myEncoderFloat(const char* name, const ImVec2& size, float* v)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::InvisibleButton(name, size);
    ImVec2 p = ImGui::GetItemRectMin();
    ImVec2 cp = ImVec2(p.x + size.x / 2, p.y + size.y / 3);
    dl->AddCircleFilled(ImVec2(cp.x+2,cp.y+2), size.y / 6, 0xff000000);
    dl->AddCircleFilled(cp, size.y / 6, 0xffcccc | (ImGui::IsItemHovered() ? 0xcf000000 : 0x9f000000));
    dl->AddCircleFilled(ImVec2(cp.x + (float)sin(-*v * M_PI * 2 + M_PI) * size.y / 10, cp.y + (float)cos(-*v * M_PI * 2 + M_PI) * size.y / 10), 3 * gConfig.mUIScale, 0x7f000000);
    char temp[16];
    sprintf(temp," %c%03d", *v < 0 ? '-' : '+', ((int)abs(*v))%1000);
    dl->AddText(ImVec2(p.x, p.y + size.y - 32 * gConfig.mUIScale), 0x7fffcccc, temp);
    sprintf(temp, " .%03d", (int)abs((*v - (int)*v) * 1000));
    dl->AddText(ImVec2(p.x, p.y + size.y - 16 * gConfig.mUIScale), 0x7fffcccc, temp);
    if (ImGui::IsItemActive())
    {
        ImVec2 mp = ImGui::GetMouseDragDelta();
        float delta = (mp.y * -0.001f) / gConfig.mUIScale;
        delta *= delta * delta;
        *v += delta;
    }
    if (ImGui::IsItemClicked(1))
        *v = 0;

    return ImGui::IsItemActive();
}

void uibar(ImVec2 mainwindoriginalsize)
{
    /*
      UI elements are either so atomic (single value) or
      so deeply buffered that audio mutex isn't needed.
    */
    ImVec2 uiwindoriginalsize = mainwindoriginalsize;
    uiwindoriginalsize.y = 150 * gConfig.mUIScale;
    ImGui::SetNextWindowPos(ImVec2(0, mainwindoriginalsize.y + 18 * gConfig.mUIScale));
    ImGui::SetNextWindowSize(uiwindoriginalsize);

    ImGui::Begin("UI", 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    for (int cell = 0; cell < MAXVAR; cell++)
    {
        if (gCelldata[cell].mDynamic && gCelldata[cell].mCode != 0)
        {
            Op* o = gCelldata[cell].mCode;
            int memofs = 0;
            char *cellname = gCelldata[cell].mLabel;
            if (cellname[0] == 0)
                cellname = gCelldata[cell].mDefaultLabel;
            //sprintf(cellname, "%c%d", 'A' + (cell % 27), (cell / 27) + 1);

            int element = 0;
            while (o->mOpcode)
            {
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_SLIDER || o->mArg.i.i == FUNC_SLIDERV) && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%ds%d", cell, memofs);
                    //if (ImGui::VSliderFloat(temp, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs), 0, 1))
                    if (mySliderFloat(temp, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs), 0, 1))
                    {
                        updateuistore(cell, element, *(float*)(gCelldata[cell].mDynmem + memofs));
                    }
                    element++;
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_ENCODER || o->mArg.i.i == FUNC_ENCODERV) && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(48 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%ds%d", cell, memofs);
                    //if (ImGui::VSliderFloat(temp, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs), 0, 1))
                    if (myEncoderFloat(temp, ImVec2(48 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs)))
                    {
                        updateuistore(cell, element, *(float*)(gCelldata[cell].mDynmem + memofs));
                    }
                    element++;
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_GRAPH && gCelldata[cell].mDynmem)
                {
                    char temp[441];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%dg%d", cell, memofs);
                    int mindata = 441;
                    float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int) * 2);
                    int graphindex = *(int*)(gCelldata[cell].mDynmem + memofs);
                    int graphflags = *(int*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
                    char* sync = "off";
                    char* zoom;
                    switch (graphflags & 3)
                    {
                    default:
                    case 0: mindata = gSamplerate / 100; zoom = "10ms"; break;
                    case 1: mindata = gSamplerate / 10; zoom = "100ms"; break;
                    case 2: mindata = gSamplerate; zoom = "1s"; break;
                    case 3: mindata = gSamplerate / 1000; zoom = "1ms"; break;
                    }

                    int ofs = (graphindex + (1 << 17) - mindata) % (1 << 17);
                    if (!(graphflags & 4))
                    {
                        sync = "sync ^";
                        int over = 0;
                        while (over < (1 << 17) && graphdata[((1 << 17) - over + ofs) % (1 << 17)] < 0) over++;
                        int under = over;
                        while (under < (1 << 17) && graphdata[((1 << 17) - under + ofs) % (1 << 17)] > 0) under++;
                        ofs = (ofs + (1 << 17) - under) % (1 << 17);
                    }
                    else
                        if (!(graphflags & 8))
                        {
                            sync = "sync v";
                            int under = 0;
                            while (under < (1 << 17) && graphdata[((1 << 17) - under + ofs) % (1 << 17)] > 0) under++;
                            int over = under;
                            while (over < (1 << 17) && graphdata[((1 << 17) - over + ofs) % (1 << 17)] < 0) over++;
                            ofs = (ofs + (1 << 17) - over) % (1 << 17);
                        }
                    float tempf[120];
                    for (int i = 0; i < 120; i++)
                        tempf[i] = graphdata[((i * mindata) / 120 + ofs) % (1 << 17)];
                    myPlotLines(temp, tempf, 120, -1, 1, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    //ImGui::PlotLines(temp, tempf, 120, 0, 0, -1, 1, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    ImGui::Text("%s", cellname);
                    ImGui::SameLine();
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
                    if (ImGui::Button(sync, ImVec2(48 * gConfig.mUIScale, 16 * gConfig.mUIScale)))
                    {
                        switch ((graphflags >> 2) & 3)
                        {
                        case 0: // 00 -> 01
                            graphflags ^= 4;
                            break;
                        case 1: // 01 -> 11
                            graphflags ^= 8;
                            break;
                        case 2: // 11 -> 00
                            graphflags ^= 4 + 8;
                            break;
                        default:
                            graphflags &= ~(3 << 2);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(zoom, ImVec2(48 * gConfig.mUIScale, 16 * gConfig.mUIScale)))
                    {
                        graphflags = (graphflags & ~3) | (((graphflags & 3) + 1) & 3);
                    }
                    ImGui::PopStyleVar(1);
                    *(int*)(gCelldata[cell].mDynmem + memofs + sizeof(int)) = graphflags;
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_FFT && gCelldata[cell].mDynmem)
                {
                    char temp[441];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%dg%d", cell, memofs);
                    float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
                    SDL_LockMutex(gAudioMutex);
                    int graphindex = *(int*)(gCelldata[cell].mDynmem + memofs);
                    float tempf[FFTSIZE];
                    for (int i = 0; i < FFTSIZE / 2; i++)
                    {
                        tempf[i * 2] = graphdata[(i + graphindex) % (FFTSIZE)];
                        tempf[i * 2 + 1] = 0;
                    }
                    SDL_UnlockMutex(gAudioMutex);
                    float tempi[FFTSIZE];
                    gFft_object.do_fft(tempi, tempf);
                    for (int i = 0; i < FFTSIZE/4; i++)
                    {
                        tempf[i] = sqrt(tempi[i * 2 + 0] * tempi[i * 2 + 0] + tempi[i * 2 + 1] * tempi[i * 2 + 1]);
                    }

                    for (int i = 0; i < 120; i++)
                    {
                        float ppos = powf(i / 120.0f, 2.0f) * (FFTSIZE / 8);

                        float f = ppos - (int)ppos;
                        float a = i ? tempf[(int)ppos - 1] : 0;
                        float b = tempf[(int)ppos];
                        float c = i < 441 ? tempf[(int)ppos + 1] : 0;
                        float d = i < 440 ? tempf[(int)ppos + 2] : 0;

                        tempi[i] = (float)EvalFunc::catmullrom(f, a, b, c, d);

                    }

                    //ImGui::PlotLines(temp, tempf, 120, 0, 0, 0, 64, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    myPlotLines(temp, tempi, 120, 0, 64, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_PLOTXY || o->mArg.i.i == FUNC_PLOTXY2) && gCelldata[cell].mDynmem)
                {
                    char temp[441];
                    sprintf(temp, "###%dpf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%dp%d", cell, memofs);
                    float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    ImVec2 p = ImGui::GetItemRectMin();
                    for (int i = 0; i < 1024; i++)
                        ImGui::GetWindowDrawList()->AddCircleFilled(
                            ImVec2(p.x + (graphdata[i * 2 + 0] + 0.5f) * 110, p.y + (graphdata[i * 2 + 1] + 0.5f) * 100),
                            //ImVec2(p.x + (graphdata[i * 2 + 0] + 0.5f) * 110 + 1, p.y + (graphdata[i * 2 + 1] + 0.5f) * 100),
                            1,
                            0x3fffcccc);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_BUTTON && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    ImGui::Dummy(ImVec2(0, 40 * gConfig.mUIScale));
                    sprintf(temp, "X###%db%d", cell, memofs);
                    //ImGui::Button(temp, ImVec2(24 * gConfig.mUIScale, 20 * gConfig.mUIScale));
                    
                    *(bool*)(gCelldata[cell].mDynmem + memofs) = myButton(temp);
                    ImGui::Dummy(ImVec2(0, 32 * gConfig.mUIScale));
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_TOGGLE || o->mArg.i.i == FUNC_TOGGLEV) && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dtf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    ImGui::Dummy(ImVec2(0, 41 * gConfig.mUIScale));
                    sprintf(temp, "###%dt%d", cell, memofs);                    
                    /*
                    if (ImGui::Checkbox(temp, (bool*)(gCelldata[cell].mDynmem + memofs)))
                    {
                        updateuistore(cell, element, *(bool*)(gCelldata[cell].mDynmem + memofs) ? 1.0f : 0.0f);
                    }
                    */
                    bool* state = (bool*)(gCelldata[cell].mDynmem + memofs);

                    if (myToggle(temp, *state))
                    {
                        *state = !*state;
                        updateuistore(cell, element, *state ? 1.0f : 0.0f);
                    }
                    
                    element++;
                    ImGui::Dummy(ImVec2(0, (32-6) * gConfig.mUIScale));
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_BAR && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dtf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%dt%d", cell, memofs);
                    // a bit of an overkill, but, eh, imgui doesn't have vertical progress bars
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xff9f9f9f);
                    ImGui::PlotHistogram(temp, (float*)(gCelldata[cell].mDynmem + memofs), 1, 0, 0, 0, 1, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                    ImGui::PopStyleColor(1);
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_BARA && gCelldata[cell].mDynmem)
                {
                    float bardata[20];
                    int n = 0;
                    int* coords = (int*)(gCelldata[cell].mDynmem + memofs);
                    int x0, y0, x1, y1;
                    if (coords[0] == 0 || coords[1] == 0)
                    {
                        x0 = y0 = x1 = y1 = 0;
                    }
                    else
                    {
                        cell_to_xy(coords[0], x0, y0);
                        cell_to_xy(coords[1], x1, y1);
                        x0--;
                        x1--;
                        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
                        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
                    }
                    for (int i = y0; n < 20 && i <= y1; i++)
                    {
                        for (int j = x0; n < 20 && j <= x1; j++)
                        {
                            int c = i * 27 + j;
                            if (!gCelldata[c].mText)
                                bardata[n++] = (float)gCellvalue[c];
                        }
                    }
                    if (n)
                    {
                        char temp[256];
                        sprintf(temp, "###%dba%d", cell, memofs);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                        ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                        sprintf(temp, "###%dbaf%d", cell, memofs);
                        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xff9f9f9f);
                        ImGui::PlotHistogram(temp, bardata, n, 0, 0, 0, 1, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                        ImGui::PopStyleColor(1);
                        ImGui::Text("%s", cellname);
                        ImGui::EndChild();
                        ImGui::PopStyleColor(1);
                        ImGui::SameLine();
                    }
                }

                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_PIE && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dpi%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%dpie%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale), false, 0);
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 p = ImGui::GetItemRectMin();
                    ImVec2 pt[64];
                    int pts = 0;
                    pt[pts++] = ImVec2(p.x + (60 + 5) * gConfig.mUIScale, p.y + (50 + 5) * gConfig.mUIScale); // center
                    float v = *(float*)(gCelldata[cell].mDynmem + memofs);
                    if (v < -1) v = -1;
                    if (v > 1) v = 1;
                    v *= 2 * (float)M_PI;
                    if (v > 0)
                    {
                        for (int i = 0; i < 63; i++)
                        {
                            pt[pts++] = ImVec2(p.x + (60 + 40 * (float)cos((i / 62.0) * v - (float)M_PI / 2) + 5) * gConfig.mUIScale, 
                                               p.y + (50 + 40 * (float)sin((i / 62.0) * v - (float)M_PI / 2) + 5) * gConfig.mUIScale);
                        }
                    }
                    else
                    {
                        for (int i = 62; i >=0; i--)
                        {
                            pt[pts++] = ImVec2(p.x + (60 + 40 * (float)cos((i / 62.0) * v - (float)M_PI / 2) + 5) * gConfig.mUIScale, 
                                               p.y + (50 + 40 * (float)sin((i / 62.0) * v - (float)M_PI / 2) + 5) * gConfig.mUIScale);
                        }
                    }

                    dl->AddConvexPolyFilled(pt, pts, 0xaf000000);

                    for (int i = 0; i < 64; i++)
                        pt[i] = ImVec2(pt[i].x - 5 * gConfig.mUIScale, pt[i].y - 5 * gConfig.mUIScale);

                    dl->AddConvexPolyFilled(pt, pts, 0xaf0000ff);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }

                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_PIEA && gCelldata[cell].mDynmem)
                {
                    float bardata[20];
                    int n = 0;
                    int* coords = (int*)(gCelldata[cell].mDynmem + memofs);
                    int x0, y0, x1, y1;
                    if (coords[0] == 0 || coords[1] == 0)
                    {
                        x0 = y0 = x1 = y1 = 0;
                    }
                    else
                    {
                        cell_to_xy(coords[0], x0, y0);
                        cell_to_xy(coords[1], x1, y1);
                        x0--;
                        x1--;
                        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
                        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
                    }

                    for (int i = y0; n < 20 && i <= y1; i++)
                    {
                        for (int j = x0; n < 20 && j <= x1; j++)
                        {
                            int c = i * 27 + j;
                            if (!gCelldata[c].mText)
                            {
                                if ((float)gCellvalue[c] < 0)
                                    bardata[n++] = 0;
                                else
                                    bardata[n++] = (float)gCellvalue[c];
                            }
                        }
                    }
                    float sum = 0;
                    for (int i = 0; i < n; i++)
                    {
                        sum += bardata[i];
                    }

                    if (n)
                    {
                        char temp[256];
                        sprintf(temp, "###%dpie%d", cell, memofs);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                        ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                        sprintf(temp, "###%dpiea%d", cell, memofs);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                        ImGui::BeginChild(temp, ImVec2(120 * gConfig.mUIScale, 100 * gConfig.mUIScale), false, 0);
                        if (sum)
                        {
                            float v0 = 0;
                            for (int pass = 0; pass < 2; pass++)
                            for (int i = 0; i < n; i++)
                            {
                                ImDrawList* dl = ImGui::GetWindowDrawList();
                                ImVec2 p = ImGui::GetItemRectMin();
                                ImVec2 pt[22];
                                int pts = 0;
                                float v = bardata[i] / sum;
                                v *= 2 * (float)M_PI;
                                pt[pts++] = ImVec2(p.x + (60 + 5 * cos(v0 + v/2 - (float)M_PI / 2) + 5 * (1-pass)) * gConfig.mUIScale,
                                                   p.y + (50 + 5 * sin(v0 + v/2 - (float)M_PI / 2) + 5 * (1-pass)) * gConfig.mUIScale); // center
                                for (int j = 0; j < 21; j++)
                                {
                                    pt[pts++] = ImVec2(p.x + (60 + 5 * cos(v0 + v/2 - (float)M_PI / 2) + 40 * (float)cos(v0 + (j / 20.0) * v - (float)M_PI / 2) + 5*(1-pass)) * gConfig.mUIScale,
                                                       p.y + (50 + 5 * sin(v0 + v/2 - (float)M_PI / 2) + 40 * (float)sin(v0 + (j / 20.0) * v - (float)M_PI / 2) + 5*(1-pass)) * gConfig.mUIScale);
                                }
                                v0 += v;
                                const unsigned int piecol[20] = {
                                    0xaf0000ff, 0xaf00ff00, 0xafff0000, 0xafff00ff, 0xafffff00, 0xaf00ffff, 0xafffffff,
                                    0xaf000080, 0xaf008000, 0xaf800000, 0xaf800080, 0xaf808000, 0xaf008080, 0xaf808080,
                                    0xaf8080ff, 0xaf80ff80, 0xafff8080, 0xafff80ff, 0xafffff80, 0xaf80ffff
                                };
                                if (pass == 0)
                                    dl->AddConvexPolyFilled(pt, pts, 0xaf000000);
                                else
                                    dl->AddConvexPolyFilled(pt, pts, piecol[i]);
                            }
                        }
                        ImGui::EndChild();
                        ImGui::PopStyleColor(1);
                        ImGui::Text("%s", cellname);
                        ImGui::EndChild();
                        ImGui::PopStyleColor(1);
                        ImGui::SameLine();
                    }
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_SLIDERPOT || o->mArg.i.i == FUNC_SLIDERPOTV) && gCelldata[cell].mDynmem)
                {
                    int i = 0;
                    while (i < 128 && (gMidi_pot_changed[i] == 0 || gMidi_pot_cell[i] != cell)) i++;
                    if (i != 128)
                    {
                        int v = gMidi_pot_cell[i];
                        if (v >= 0 && v < 128)
                        {
                            ImGui::SetWindowFocus("UI"); // if text editor has focus, it'll undo our changes
                            updateuistore(cell, element, (float)gMidi_pot[i]);
                            *(float*)(gCelldata[cell].mDynmem + memofs) = (float)gMidi_pot[i]; // update slider
                            gMidi_pot_changed[i] = 0;
                        }
                    }
                    char temp[256];
                    sprintf(temp, "###%dbf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    sprintf(temp, "###%ds%d", cell, memofs);
                    //if (ImGui::VSliderFloat(temp, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs), 0, 1))
                    if (mySliderFloat(temp, ImVec2(24 * gConfig.mUIScale, 100 * gConfig.mUIScale), (float*)(gCelldata[cell].mDynmem + memofs), 0, 1))
                    {
                        updateuistore(cell, element, *(float*)(gCelldata[cell].mDynmem + memofs));
                    }
                    element++;
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_TOGGLEPOT || o->mArg.i.i == FUNC_TOGGLEPOTV) && gCelldata[cell].mDynmem)
                {
                    char temp[256];
                    sprintf(temp, "###%dtf%d", cell, memofs);
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                    ImGui::BeginChild(temp, ImVec2(24 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                    ImGui::Dummy(ImVec2(0, 41 * gConfig.mUIScale));
                    sprintf(temp, "###%dt%d", cell, memofs);
                    bool* state = (bool*)(gCelldata[cell].mDynmem + memofs);
                    if (myToggle(temp, *state))
                    {
                        *state = !*state;
                        updateuistore(cell, element, *state ? 1.0f : 0.0f);
                    }
                    element++;
                    ImGui::Dummy(ImVec2(0, (32-6) * gConfig.mUIScale));
                    ImGui::Text("%s", cellname);
                    ImGui::EndChild();
                    ImGui::PopStyleColor(1);
                    ImGui::SameLine();
                }
                if (o->mOpcode == 'F' && (o->mArg.i.i == FUNC_MIDIPOT || o->mArg.i.i == FUNC_MIDIPOTV) && gCelldata[cell].mDynmem)
                {
                    int i = 0;
                    while (i < 128 && (gMidi_pot_changed[i] == 0 || gMidi_pot_cell[i] != cell)) i++;
                    if (i != 128)
                    {
                        int v = gMidi_pot_cell[i];
                        if (v >= 0 && v < 128)
                        {
                            ImGui::SetWindowFocus("UI"); // if text editor has focus, it'll undo our changes
                            updateuistore(cell, element, (float)gMidi_pot[i]);
                            gMidi_pot_changed[i] = 0;
                        }
                    }
                    element++;
                }
                if (o->mOpcode == 'F' && o->mArg.i.i == FUNC_IMG && gCelldata[cell].mDynmem)
                {
                    ImgResource* r = (ImgResource*)getResource(gCelldata[cell].mTextStore + o->mArg.i.j, 0, 1);
                    if (r && r != (ImgResource*)-1)
                    {
                        char temp[256];
                        sprintf(temp, "###%dim%d", cell, memofs);
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0xff3f1f1f);
                        ImGui::BeginChild(temp, ImVec2(100 * gConfig.mUIScale, 120 * gConfig.mUIScale), false, 0);
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        ImVec2 p = ImGui::GetItemRectMin();
                        // Since we can't load the texture from another thread, we'll just do it here
                        if (r->glhandle == 0xcccccccc)
                        {
                            int* temptex = new int[256 * 256];
                            for (int i = 0; i < 256; i++)
                            {
                                for (int j = 0; j < 256; j++)
                                {
                                    temptex[i * 256 + j] = r->imgdata[i * 256 + j] * 0x010101 | 0xff000000;
                                }
                            }
                            r->glhandle = ImGui_ImplOpenGL2_LoadTexture((const unsigned char*)temptex, 256, 256);
                            delete[] temptex;
                        }


                        ImGui::Image((ImTextureID)r->glhandle, ImVec2(100 * gConfig.mUIScale, 100 * gConfig.mUIScale));
                        ImGui::Text("%s", cellname);
                        double* coorddata = (double*)(gCelldata[cell].mDynmem + memofs);
                        float x = (float)(coorddata[0] - (int)coorddata[0]);
                        float y = (float)(coorddata[1] - (int)coorddata[1]);
                        dl->AddLine(
                            ImVec2(p.x + (x * 100 - 3) * gConfig.mUIScale, p.y + (y * 100 + 2) * gConfig.mUIScale),
                            ImVec2(p.x + (x * 100 + 6) * gConfig.mUIScale, p.y + (y * 100 + 2) * gConfig.mUIScale),
                            0xff000000);
                        dl->AddLine(
                            ImVec2(p.x + (x * 100 + 2) * gConfig.mUIScale, p.y + (y * 100 - 3) * gConfig.mUIScale),
                            ImVec2(p.x + (x * 100 + 2) * gConfig.mUIScale, p.y + (y * 100 + 6) * gConfig.mUIScale),
                            0xff000000);
                        dl->AddLine(
                            ImVec2(p.x + (x * 100 - 4) * gConfig.mUIScale, p.y + (y * 100) * gConfig.mUIScale),
                            ImVec2(p.x + (x * 100 + 5) * gConfig.mUIScale, p.y + (y * 100) * gConfig.mUIScale),
                            0xff0000ff);
                        dl->AddLine(
                            ImVec2(p.x + (x * 100) * gConfig.mUIScale, p.y + (y * 100 - 4) * gConfig.mUIScale),
                            ImVec2(p.x + (x * 100) * gConfig.mUIScale, p.y + (y * 100 + 5) * gConfig.mUIScale),
                            0xff0000ff);
                        ImGui::EndChild();
                        ImGui::PopStyleColor(1);
                        ImGui::SameLine();
                    }
                }
                if (o->mOpcode == 'F')
                    memofs += gFunc[o->mArg.i.i].mMemory + gFunc[o->mArg.i.i].mSamplerateMemory * gSamplerate;
                o++;
            }
        }
    }

    ImGui::End();
}
