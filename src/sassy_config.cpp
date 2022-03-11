#include "sassy.h"

SassyConfig::SassyConfig()
{
    mIniFile.set("sassy.ini");
    init();
}

void SassyConfig::init()
{
    mConfig.clear();
    mLastFilename = _strdup("");

    gSamplerate = 44100;
    mDt = 1.0 / 44100;
    mMidiRate = 200;
    mShowMidiMessages = 1;
    mShowCPU = 1;
    mShowCPUPeak = 0;
    mShowCPUAvg = 0;
    mShowUsedBy = 1;
    mShowUses = 0;
    mMidiChannel = -1;
    mBigKeyboard = 0;
    mUIScale = 1;
    mReloadLast = 0;
    mSyntaxColoring = 1;
    mBraceColoring = 1;
    mBitDisplayWidth = 8;

    mResourceResampler = SRC_SINC_BEST_QUALITY;
    mCaptureResampler = SRC_LINEAR;
}

#define LOAD_IF_SET(x,a,b) if (mConfig.has(a) && mConfig[a].has(b)) x = std::stoi(mConfig[a][b]);

void SassyConfig::load()
{
    init();
    mIniFile.read(mConfig);
    LOAD_IF_SET(gSamplerate, "general", "samplerate");
    LOAD_IF_SET(mMidiRate, "midi", "midi_send_rate");
    LOAD_IF_SET(mShowMidiMessages, "ui", "show_midi_messages");
    LOAD_IF_SET(mShowCPU, "ui", "show_cpu_use");
    LOAD_IF_SET(mShowCPUPeak, "ui", "show_cpu_peak");
    LOAD_IF_SET(mShowCPUAvg, "ui", "show_cpu_average");
    LOAD_IF_SET(mMidiChannel, "midi", "input_channel");
    LOAD_IF_SET(mShowUsedBy, "ui", "show_used_by");
    LOAD_IF_SET(mShowUses, "ui", "show_uses");
    LOAD_IF_SET(mBigKeyboard, "ui", "big_keyboard");
    LOAD_IF_SET(mSyntaxColoring, "ui", "syntax_coloring");
    LOAD_IF_SET(mBraceColoring, "ui", "brace_coloring");
    LOAD_IF_SET(mReloadLast, "general", "reload_last");
    LOAD_IF_SET(mResourceResampler, "general", "resource_resampler");
    LOAD_IF_SET(mCaptureResampler, "general", "capture_resampler");
    int uiscale = 1;
    LOAD_IF_SET(uiscale, "ui", "ui_scale");
    mUIScale = (float)uiscale;
    LOAD_IF_SET(mBitDisplayWidth, "ui", "bit_display_width");
    if (mConfig.has("general") && mConfig["general"].has("last_file"))
    {
        free(mLastFilename);
        mLastFilename = _strdup(mConfig["general"]["last_file"].c_str());
    }

    if (gSamplerate < 1) gSamplerate = 1;
    mDt = 1.0 / gSamplerate;
}

void SassyConfig::save()
{
    mConfig["ui"]["show_midi_messages"] = std::to_string(mShowMidiMessages ? 1 : 0);
    mConfig["ui"]["show_cpu_use"] = std::to_string(mShowCPU ? 1 : 0);
    mConfig["ui"]["show_cpu_peak"] = std::to_string(mShowCPUPeak ? 1 : 0);
    mConfig["ui"]["show_cpu_average"] = std::to_string(mShowCPUAvg ? 1 : 0);
    mConfig["ui"]["show_used_by"] = std::to_string(mShowUsedBy ? 1 : 0);
    mConfig["ui"]["show_uses"] = std::to_string(mShowUses ? 1 : 0);
    mConfig["ui"]["ui_scale"] = std::to_string(mUIScale);
    mConfig["ui"]["syntax_coloring"] = std::to_string(mSyntaxColoring ? 1 : 0);
    mConfig["ui"]["brace_coloring"] = std::to_string(mBraceColoring ? 1 : 0);
    mConfig["ui"]["big_keyboard"] = std::to_string(mBigKeyboard ? 1 : 0);
    mConfig["ui"]["bit_display_width"] = std::to_string(mBitDisplayWidth);
    mConfig["general"]["samplerate"] = std::to_string(gSamplerate);
    mConfig["general"]["reload_last"] = std::to_string(mReloadLast);
    mConfig["midi"]["midi_send_rate"] = std::to_string(mMidiRate);
    mConfig["midi"]["input_channel"] = std::to_string(mMidiChannel);
    mConfig["general"]["last_file"] = mLastFilename;
    mConfig["general"]["resource_resampler"] = std::to_string(mResourceResampler);
    mConfig["general"]["capture_resampler"] = std::to_string(mCaptureResampler);
    mIniFile.generate(mConfig);
}

void config_dialog()
{

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    int current = -1;

    if (ImGui::BeginPopupModal("Preferences", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::BeginTabBar("Preftabs");
        if (ImGui::BeginTabItem("General"))
        {
            int samplerates[12] = { 384000, 352800, 192000, 176400, 96000, 88200, 48000, 44100, 22050, 16000, 11025, 8000 };
            current = -1;
            for (int i = 0; i < 12; i++)
                if (gSamplerate == samplerates[i])
                    current = i;
            
            if (ImGui::Combo("Sample rate", &current, " 384000\0 352800\0 192000\0 176400\0  96000\0  88200\0  48000\0  44100\0  22050\0  16000\0  11025\0   8000\0\0"))
            {
                SDL_LockMutex(gAudioMutex);
                nukeResources();
                gSamplerate = samplerates[current];
                gConfig.mDt = 1.0 / gSamplerate;
                gScope->realloc();
                for (int i = 0; i < MAXVAR; i++)
                    poke(i);
                warm_reset();
                SDL_UnlockMutex(gAudioMutex);
                initaudio();
                initcapture();
            }
            ImGui::Text(
                "Note! Higher sample rates require more CPU power.\n"
                "This is the master sample rate at which everything\n"
                "is run; it's output rate, capture rate, etc.\n");
            ImGui::Separator();
            if (ImGui::BeginCombo("Audio Driver", gConfig.mConfig["general"]["audio_driver"].length() == 0 ? "(default)" : gConfig.mConfig["general"]["audio_driver"].c_str(), 0))
            {
                if (ImGui::Selectable("(default)"))
                {
                    gConfig.mConfig["general"]["audio_driver"] = "";
                    SDL_AudioInit(SDL_GetAudioDriver(0));
                    initaudio();
                    initcapture();
                }
                for (int n = 0; n < SDL_GetNumAudioDrivers(); n++)
                {
                    if (ImGui::Selectable(SDL_GetAudioDriver(n)))
                    {
                        gConfig.mConfig["general"]["audio_driver"] = SDL_GetAudioDriver(n);
                        SDL_AudioInit(gConfig.mConfig["general"].get("audio_driver").c_str());
                        initaudio();
                        initcapture();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Leave at default unless you have issues.");
            ImGui::Separator();
            if (ImGui::BeginCombo("Output device", gConfig.mConfig["general"]["output_device"].length() == 0 ? "(system default)" : gConfig.mConfig["general"]["output_device"].c_str(), 0))
            {
                if (ImGui::Selectable("(system default)"))
                {
                    gConfig.mConfig["general"]["output_device"] = "";
                    initaudio();
                }
                for (int n = 0; n < SDL_GetNumAudioDevices(0); n++)
                {
                    if (ImGui::Selectable(SDL_GetAudioDeviceName(n, 0)))
                    {
                        gConfig.mConfig["general"]["output_device"] = SDL_GetAudioDeviceName(n, 0);
                        initaudio();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Actual output rate: %d%s", gActiveAudioSpec.freq, gActiveAudioSpec.freq == gSamplerate ? "" : " - resampling will occur");
            ImGui::Separator();
            if (ImGui::BeginCombo("Capture device", gConfig.mConfig["general"]["input_device"].length() == 0 ? "(disabled)" : gConfig.mConfig["general"]["input_device"].c_str(), 0))
            {
                if (ImGui::Selectable("(disabled)"))
                {
                    gConfig.mConfig["general"]["input_device"] = "";
                    initcapture();
                }
                for (int n = 0; n < SDL_GetNumAudioDevices(1); n++)
                {
                    if (ImGui::Selectable(SDL_GetAudioDeviceName(n, 1)))
                    {
                        gConfig.mConfig["general"]["input_device"] = SDL_GetAudioDeviceName(n, 1);
                        initcapture();
                    }
                }
                ImGui::EndCombo();
            }
            if (gConfig.mConfig["general"]["input_device"] == "")
                ImGui::Text("Capture disabled");
            else
                ImGui::Text("Actual capture rate: %d%s", gActiveCaptureSpec.freq, gActiveCaptureSpec.freq == gSamplerate ? "" : " - resampling will occur");
            ImGui::Separator();
            if (ImGui::BeginCombo("Resource resampler", src_get_name(gConfig.mResourceResampler)))
            {
                for (int n = 0; n < 5; n++)
                {
                    if (ImGui::Selectable(src_get_name(n)))
                    {
                        gConfig.mResourceResampler = n;
                        nukeResources();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("If sample loading takes forever, change this.");
            ImGui::Separator();
            if (ImGui::BeginCombo("Capture resampler", src_get_name(gConfig.mCaptureResampler)))
            {
                for (int n = 0; n < 5; n++)
                {
                    if (ImGui::Selectable(src_get_name(n)))
                    {
                        gConfig.mCaptureResampler = n;
                        initcapture();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Careful with the heavier resamplers. This is for real-time use.");

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MIDI & ASIO"))
        {

            if (ImGui::BeginCombo("MIDI Input device", gConfig.mConfig["midi"]["input_device"].length() == 0 ? "(disabled)" : gConfig.mConfig["midi"]["input_device"].c_str(), 0))
            {
                if (ImGui::Selectable("(disabled)"))
                {
                    gConfig.mConfig["midi"]["input_device"] = "";
                    initmidi();
                }
                for (int n = 0; n < (signed)gRtmidi_in->getPortCount(); n++)
                {
                    if (ImGui::Selectable(gRtmidi_in->getPortName(n).c_str()))
                    {
                        gConfig.mConfig["midi"]["input_device"] = gRtmidi_in->getPortName(n).c_str();
                        initmidi();
                    }
                }
                ImGui::EndCombo();
            }
            current = gConfig.mMidiChannel + 1;
            if (ImGui::Combo("MIDI input channel", &current, "Omni\0   0\0   1\0   2\0   3\0   4\0   5\0   6\0   7\0   8\0   9\0  10\0  11\0  12\0  13\0  14\0  15\0\0"))
            {
                gConfig.mMidiChannel = current - 1;
            }

            if (ImGui::BeginCombo("MIDI Output device", gConfig.mConfig["midi"]["output_device"].length() == 0 ? "(disabled)" : gConfig.mConfig["midi"]["output_device"].c_str(), 0))
            {
                if (ImGui::Selectable("(disabled)"))
                {
                    gConfig.mConfig["midi"]["output_device"] = "";
                    initmidi();
                }
                for (int n = 0; n < (signed)gRtmidi_out->getPortCount(); n++)
                {
                    if (ImGui::Selectable(gRtmidi_out->getPortName(n).c_str()))
                    {
                        gConfig.mConfig["midi"]["output_device"] = gRtmidi_out->getPortName(n).c_str();
                        initmidi();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SliderInt("MIDI output rate", &gConfig.mMidiRate, 10, 1000);
            ImGui::Text("Note! Too high MIDI out rate may cause stalling.");
            ImGui::Separator();
            ImGui::Text("Note: ASIO support is not working yet.");
            ImGui::Separator();
            if (ImGui::BeginCombo("ASIO device", gConfig.mConfig["general"]["asio_device"].length() == 0 ? "(disabled)" : gConfig.mConfig["general"]["asio_device"].c_str(), 0))
            {
                if (ImGui::Selectable("(disabled)"))
                {
                    gConfig.mConfig["general"]["asio_device"] = "";
                    initasio();
                }
                for (int n = 0; n < asio_devicecount(); n++)
                {
                    if (ImGui::Selectable(asio_devicename(n)))
                    {
                        gConfig.mConfig["general"]["asio_device"] = asio_devicename(n);
                        initasio();
                    }
                }
                ImGui::EndCombo();
            }
            if (gConfig.mConfig["general"]["input_device"] == "")
                ImGui::Text("Capture disabled");
            else
                ImGui::Text("Actual capture rate: %d%s", gActiveCaptureSpec.freq, gActiveCaptureSpec.freq == gSamplerate ? "" : " - resampling will occur");
            ImGui::Separator();


            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("UI"))
        {
            ImGui::Checkbox("Reload last file on start", &gConfig.mReloadLast);
            ImGui::Checkbox("Use big virtual MIDI keyboard", &gConfig.mBigKeyboard);
            ImGui::Separator();
            ImGui::Checkbox("Use syntax coloring", &gConfig.mSyntaxColoring);
            ImGui::Checkbox("Use brace coloring", &gConfig.mBraceColoring);
            ImGui::Separator();
            ImGui::Checkbox("Show MIDI messages", &gConfig.mShowMidiMessages);
            ImGui::Checkbox("Show CPU use", &gConfig.mShowCPU);
            ImGui::Checkbox("Show CPU peaks", &gConfig.mShowCPUPeak);
            ImGui::Checkbox("Show CPU average", &gConfig.mShowCPUAvg);
            ImGui::Separator();
            ImGui::Checkbox("Show \"used by\" lines", &gConfig.mShowUsedBy);
            ImGui::Checkbox("Show \"useds\" lines", &gConfig.mShowUses);
            ImGui::Separator();
            ImGui::Text("UI scale ");
            ImGui::SameLine();
            int uiscale = (int)gConfig.mUIScale;
            ImGui::RadioButton("1x", &uiscale, 1);
            ImGui::SameLine();
            ImGui::RadioButton("2x", &uiscale, 2);
            ImGui::SameLine();
            ImGui::RadioButton("3x", &uiscale, 3);
            ImGui::SameLine();
            ImGui::RadioButton("4x", &uiscale, 4);
            gConfig.mUIScale = (float)uiscale;
            ImGui::Separator();
            current = gConfig.mBitDisplayWidth == 16 ? 1 :
                      gConfig.mBitDisplayWidth == 24 ? 2 :
                      gConfig.mBitDisplayWidth == 32 ? 3 : 0;
            if (ImGui::Combo("Bit display width", &current, "  8 bits\0 16 bits\0 24 bits\0 32 bits\0\0"))
            {
                gConfig.mBitDisplayWidth = current == 1 ? 16 :
                                           current == 2 ? 24 :
                                           current == 3 ? 32 : 8;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        if (ImGui::Button("close"))
        {
            ImGui::CloseCurrentPopup();
            gConfig.save();
        }
        ImGui::EndPopup();
    }
}
