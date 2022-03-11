#include "sassy.h"
#include "data/ks_png.h"

#include "stb/stb_image.h"

unsigned int ImGui_ImplOpenGL2_LoadTexture(const unsigned char* data, int w, int h);

static unsigned long long ks_texture = 0;

int gVirtualKeyboardWasDown = -1;


static char keytomidinote[57] =
{
    48,   50,   52,53,   55,   57,   59,60,   62,   64,65,   67,   69,   71,72,   74,   76,77,   79,   81,   83,84,   86,   88,89,   91,   93,   95,96,
       49,   51, -1,  52,   56,   58,  -1, 61,   63, -1,  66,   68,   70, -1,  73,   75, -1,  78,   80,   82, -1,  85,   87, -1,  90,   92,   94, -1
}; // 20 black keys

void do_show_keyboard_window()
{
    if (!ks_texture)
    {
        int x, y, c;
        unsigned char* data = stbi_load_from_memory(ks_png, ks_png_len, &x, &y, &c, 4);
        ks_texture = (unsigned long long)ImGui_ImplOpenGL2_LoadTexture(data, x, y);
        stbi_image_free(data);
    }

    ImGui::Begin("Virtual Keyboard", &gShowKeyboardWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImVec2 kbsize = ImVec2((gConfig.mBigKeyboard ? 3 : 1) * 256 * gConfig.mUIScale, (gConfig.mBigKeyboard ? 3 : 1) * 32 * gConfig.mUIScale);
    ImGui::BeginChild("kbd", kbsize, false, ImGuiWindowFlags_NoBackground);
    ImVec2 p = ImGui::GetItemRectMin();
    ImGui::GetWindowDrawList()->AddImage((ImTextureID)ks_texture, p, ImVec2(p.x+kbsize.x,p.y+kbsize.y));
    ImGui::InvisibleButton("click catcher", kbsize);
    int isdown = -1;

    std::vector<unsigned char> msg;
    msg.push_back(0); // note on
    msg.push_back(0); // note
    msg.push_back(0); // velocity

    if (ImGui::IsItemHovered())
    {
        if (ImGui::IsMouseDown(0))
        {
            ImVec2 mp = ImGui::GetMousePos();
            mp.x -= p.x;
            mp.y -= p.y;
            mp.x /= (gConfig.mBigKeyboard ? 3 : 1) * gConfig.mUIScale;
            mp.y /= (gConfig.mBigKeyboard ? 3 : 1) * gConfig.mUIScale;
            if (mp.x > 24) // white keys
                isdown = ((int)mp.x - 24) / 8;
            if (mp.y < 16 && mp.x > 28 && mp.x < 243) // could be a black key..
            {
                int t = ((int)mp.x - 28) / 8;
                if (!(t == 2 || t == 6 || t == 9 || t == 13 || t == 16 || t == 20 || t == 23))
                {
                    isdown = t + 29;
                }
            }
            if (mp.x > 5 && mp.x < 11)
            {
                msg[0] = 0xb0; // pot
                msg[1] = 1; // 1 mod wheel
                int v = 127 - (int)(mp.y * 4);
                if (v < 0) v = 0;
                if (v > 127) v = 127;
                msg[2] = (char)v;
                midicallback(0, &msg, 0);
            }
            if (mp.x > 13 && mp.x < 20)
            {
                msg[0] = 0xe0; // pitch bend
                msg[1] = 0; // lsb
                int v = 127 - (int)(mp.y * 4);
                if (v < 0) v = 0;
                if (v > 127) v = 127;
                msg[2] = (char)v; // msb
                midicallback(0, &msg, 0);
            }
        }
    }
    if (gVirtualKeyboardWasDown != isdown && gVirtualKeyboardWasDown != -1)
    {
        msg[0] = 0x80; // note off
        msg[1] = keytomidinote[gVirtualKeyboardWasDown];
        msg[2] = 0;
        if (keytomidinote[gVirtualKeyboardWasDown] != -1)
            midicallback(0, &msg, 0);
    }
    if (gVirtualKeyboardWasDown != isdown && isdown != -1)
    {
        msg[0] = 0x90; // note on
        msg[1] = keytomidinote[isdown];
        msg[2] = 0x7f;
        if (keytomidinote[isdown] != -1)
            midicallback(0, &msg, 0);
    }

    gVirtualKeyboardWasDown = isdown;

    ImGui::EndChild();
    
    
    /*
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text(
            " 2 3   5 6 7   \n"
            "q w e r t y u i");
        ImGui::EndTooltip();
    }
    */
    ImGui::End();
}
