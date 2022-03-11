#include "sassy.h"
#include "data/logo_png.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

unsigned int ImGui_ImplOpenGL2_LoadTexture(const unsigned char* data, int w, int h);

static unsigned long long logo_texture = 0;

void TextCenter(std::string text) {
    float font_size = ImGui::GetFontSize() * text.size() / 2;
    ImGui::SameLine(
        ImGui::GetWindowSize().x / 2 -
        font_size + (font_size / 2)
    );

    ImGui::Text(text.c_str());
    ImGui::Text("");
}

void about_dialog()
{
    if (!logo_texture)
    {
        int x, y, c;
        unsigned char *data = stbi_load_from_memory(logo_png, logo_png_len, &x, &y, &c, 4);
        logo_texture = (unsigned long long)ImGui_ImplOpenGL2_LoadTexture(data, x, y);
        stbi_image_free(data);
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopup("about_sassy", ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Image((ImTextureID)logo_texture, ImVec2(400 * gConfig.mUIScale, 263 * gConfig.mUIScale));
        ImGui::Text("");
        ImGui::Text("");
        TextCenter("\"Sassy\"");
        TextCenter("- The Audio Spreadsheet -");
        TextCenter("Version " SASSY_VERSION);
        TextCenter("Copyright 2021 by Jari Komppa");
        ImGui::Dummy(ImVec2(30, 1)); ImGui::SameLine();
        ImGui::BeginChild("Acknowledgements", ImVec2(340, 160), true, ImGuiWindowFlags_NoResize);
        ImGui::TextWrapped(
            "Welcome to Sassy!\n"
            "-----------------\n"
            "Thank you for spending time in the world of spreadsheet audio.\n"
            "\n"
            "Latest version can be found at https://sol-hsa.itch.io/sassy\n"
            "\n"
            "Is something missing? Is something weirder than it should be? "
            "Join our discussion at\nhttps://discord.com/invite/7SucFxW32n\n"
            "\n"
            "Warranty notice\n"
            "---------------\n"
            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
            "\n"
            "Aknowledgements\n"
            "---------------\n"
            "Sassy uses the following third party libraries under various open source licenses:\n"
            "\n"
            "akwf\n"
            "ayumi\n"
            "cSID\n"
            "Dear ImGui\n"
            "dr_flac\n" 
            "dr_mp3\n" 
            "dr_wav\n" 
            "fft-real\n"
            "klatt\n"
            "libsamplerate\n"
            "mINI\n"
            "padsynth\n"
            "rtmidi\n"
            "Simple DirectMedia Layer 2\n"
            "stb_hexwave\n"
            "stb_image\n"
            "stb_vorbis\n"
            "tinyfiledialogs\n"
            "Xbkyak\n" 
            "\n"
            "See 3rdparty.txt for open source license details.\n");
        ImGui::Separator();
        ImGui::TextWrapped("ASIO is a registered trademark of Steinberg Media Technologies GmbH");
        ImGui::EndChild();

        ImGui::EndPopup();
    }
}
