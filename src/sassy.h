#define _CRT_SECURE_NO_WARNINGS

/*
BUGS
- add/remove lines should move $a$1 cells
- variable patterns in "text" should not be changed

TODO
- ASIO
- copy/paste context menu
- virtual keyboard w/keyboard
- step seq through midi input
- wave recording
- clean up this mess
- launchpad api
- midi sends to its own thread, with internal midi out queue where new commands clobber old ones
(i.e, if we get a hundred pitch bend commands, only the latest one survives, but its place in queue is defined by the first one that arrived)

TODO?
- cell aliases varname:=...?
- easy polyphony mode?
- export c++?
- "classy", programming language-ish thing?
- boxing?
- investigate asio?
- paraverb?

some functions:
- togglenote
- joystick inputs
- launchpad related stuff
*/

#include <chrono>
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "eval.h"
#include "RtMidi.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include "well512.h"
//#include "FFTRealFixLen.h"
#include "FFTReal.h"
#include "libsamplerate/samplerate.h"
#include "xbyak/xbyak.h"
#include "mini/ini.h"
#include <set>
#include <utility>

#define SASSY_VERSION "1.8 BETA"
#define CAPTURE_LEN 4096
#define MAX_HANDLES 1024

struct Celldata
{
    std::set<int> mUsedby;
    std::set<int> mUses;
    std::set<std::pair<int, int>> mUsesArea;
    double mHilighttime;
    int mHilightcolor;
    char* mRawtext;
    char* mDynmem;
    int mDynmemsize;
    Op* mCode;
    bool mText;
    bool mDynamic;
    char mTextStore[1024];
    char* mLabel;
    char mDefaultLabel[8];
    void init()
    {
        mUsedby.clear();
        mUses.clear();
        mUsesArea.clear();
        delete[] mDynmem;
        delete[] mCode;
        mDynmemsize = 0;
        mDynmem = 0;
        mRawtext[0] = 0;
        mText = false;
        mDynamic = true;
        mCode = 0;
        mHilighttime = -1;
        mHilightcolor = 0;
        mLabel = mDefaultLabel;
    }
    Celldata()
    {
        mDynmem = 0;
        mCode = 0;
        mRawtext = new char[1025];
        mDefaultLabel[0] = 0;
        init();
    }
    ~Celldata() { delete[] mDynmem; delete[] mRawtext; }
};

struct SassyConfig
{
    mINI::INIStructure mConfig;
    mINI::INIFile mIniFile;

    char* mLastFilename;
    double mDt;
    int mMidiRate;
    int mMidiChannel;
    float mUIScale;
    int mBitDisplayWidth;

    bool mShowMidiMessages;
    bool mShowCPU;
    bool mShowCPUPeak;
    bool mShowCPUAvg;
    bool mShowUsedBy;
    bool mShowUses;
    bool mReloadLast;
    bool mSyntaxColoring;
    bool mBraceColoring;
    bool mBigKeyboard;

    int mResourceResampler;
    int mCaptureResampler;

    SassyConfig();
    void init();
    void load();
    void save();
};

struct ScopeData
{
    struct Channel
    {
        bool mEnabled;
        float mScale;
        int mScaleSlider;
        float mOffset;
        float* mData;
        Channel()
        {
            mEnabled = true;
            mScale = 1;
            mScaleSlider = 0;
            mOffset = 0;
            mData = new float[gSamplerate * 10];
            memset(mData, 0, sizeof(float) * gSamplerate * 10);
        }
        ~Channel()
        {
            delete[] mData;
        }
    } mCh[4];
    
    void realloc()
    {
        mIndex = 0;
        for (int i = 0; i < 4; i++)
        {
            delete[] mCh[i].mData;
            mCh[i].mData = new float[gSamplerate * 10];
            memset(mCh[i].mData, 0, sizeof(float) * gSamplerate * 10);
        }
    }
    
    int mIndex;
    float mScroll;
    float mTimeScale;
    int mTimeScaleSlider;
    int mSyncMode;
    int mSyncChannel;
    int mMode;
    int mDisplay;
    int mFFTZoom;
    int mPot;
    float fft1[65536 * 2];
    float fft2[65536 * 2];
    float ffta[65536 * 2];

    ScopeData()
    {
        mPot = 0;
        mFFTZoom = 0;
        mIndex = 0;
        mScroll = 0;
        mTimeScale = 0.01f;
        mTimeScaleSlider = 0;
        mSyncMode = 0;
        mSyncChannel = 0;
        mMode = 0;
        mDisplay = 0;
    }
};

struct Resource 
{
    int handle = 0;
    char* fn = 0;
    int hash = 0;
    Resource() {};
    virtual ~Resource() { delete[] fn; }
};

struct WavResource : Resource
{
    float* data = 0;
    int channels = 0;
    int frames = 0;
    virtual ~WavResource() { delete[] data; }
};

struct ImgResource : Resource
{
    unsigned char imgdata[256 * 256];
    unsigned long long glhandle = 0;
    ImgResource() { }
    virtual ~ImgResource() { if (glhandle) glDeleteTextures(1, (GLuint*)&glhandle); }
};

unsigned int ImGui_ImplOpenGL2_LoadTexture(const unsigned char* data, int w, int h);

extern SDL_AudioSpec gActiveAudioSpec, gActiveCaptureSpec;
extern SDL_AudioDeviceID gAudioDeviceID, gCaptureDeviceID;

extern SRC_STATE* gCaptureResampler;

extern WavResource* gWavHandle[MAX_HANDLES];

extern SassyConfig gConfig;

extern ScopeData* gScope;

extern Celldata* gCelldata;

extern SDL_mutex* gAudioMutex;


extern ffft::FFTReal<float> gFft_object;

extern ffft::FFTReal<float> gFft_object_16;
extern ffft::FFTReal<float> gFft_object_32;
extern ffft::FFTReal<float> gFft_object_64;
extern ffft::FFTReal<float> gFft_object_128;
extern ffft::FFTReal<float> gFft_object_256;
extern ffft::FFTReal<float> gFft_object_512;
extern ffft::FFTReal<float> gFft_object_1024;
extern ffft::FFTReal<float> gFft_object_2048;
extern ffft::FFTReal<float> gFft_object_4096;
extern ffft::FFTReal<float> gFft_object_8192;
extern ffft::FFTReal<float> gFft_object_16384;
extern ffft::FFTReal<float> gFft_object_32768;
extern ffft::FFTReal<float> gFft_object_65536;

extern volatile int gResourceLoaderRunning;

extern RtMidiIn* gRtmidi_in;
extern RtMidiOut* gRtmidi_out;
extern double gMidiCredits;

extern int gEditorcell;
extern int gArea1, gArea2;

extern bool gMuted;

extern double* gCellvalue;
extern double* gCellvaluew;

extern bool gLaunchpadMode;

extern bool gWasdown[512];

extern double gTime;
extern float gOutputSample[2];
extern float gInputSample[2];
extern float gCPU;
extern float gCPU_peak;
extern float gCPU_avg_sum;
#define AVG_SAMPLES 200
extern float gCPU_avg_data[AVG_SAMPLES];
extern int gCPU_avg_idex;
extern WELL512 gRandom;

extern double gMidi_pot[128];
extern int gMidi_pot_changed[128];
extern int gMidi_pot_cell[128];
extern double gMidi_prog;
extern double gMidi_pitch;

extern int gMidi_noteon;
extern double gMidi_noteval;
extern double gMidi_note;
extern double gMidi_notevel;

extern int gMidi_poly_notetoindex[128 * 16];
extern int gMidi_poly_channel[128];
extern int gMidi_poly_noteon[128];
extern double gMidi_poly_noteval[128];
extern double gMidi_poly_note[128];
extern double gMidi_poly_notevel[128];

extern int gMidi_perchan_notetoindex[128 * 16];
extern int gMidi_perchan_noteon[128 * 16];
extern double gMidi_perchan_note[128 * 16];
extern double gMidi_perchan_noteval[128 * 16];
extern double gMidi_perchan_notevel[128 * 16];

extern unsigned char gOutmidivel[128 * 16];
extern double gOutmidiupdate[128 * 16];
extern int gOutmidipitch[16];
extern int gOutmidipot[128 * 16];
extern int gOutmidiprog[16];



extern const char* gNotestr[128];


extern Op gBC[1024];

extern int gActivecell[MAXVAR];
extern int gActivecells;

extern int gActiveMidiout[128 * 16];
extern int gActiveMidiouts;

extern float gCaptureData[CAPTURE_LEN * 2];
extern int gCaptureWrite;
extern int gCaptureRead;

extern double gCPUTimePerSample;

extern float gProbe[4];

extern bool gShowHelpWindow;
extern bool gShowScopeWindow;
extern bool gShowKeyboardWindow;
extern bool gShowMidiPlayerWindow;

extern char gMidimessage[256];

extern int gCopyofs;

Resource* getResource(char* fn, double parm, int type, void *parms = 0);
Resource* regetResource(char* fn, double parm, int type, void* parms = 0);
void nukeResources();
int fullfillResourceRequests(void*);
void checkResourceRequests();

void midicallback(double /*deltatime*/, std::vector< unsigned char >* message, void* /*userData*/);

char* mystrdup(const char* s);

void poke(int cell);
int is_value_prefix(char c);
void setcellvalue(int cell, double value);
void midioutscan();
void launchpad_update();
void do_show_help_window();
void do_show_scope_window();
void do_show_midiplayer_window();
void context_menu(int cell, int gArea1, int gArea2);
bool match(char* a, char* b);
void about_dialog();
void config_dialog();
void do_show_keyboard_window();
void cell_to_xy(int cell, int& x, int& y);
void initaudio();
void initcapture();
void initmidi();
void warm_reset();
void midiplayer_tick();
void uibar(ImVec2 mainwindoriginalsize);
void updateuistore(int cell, int element, float v);


std::string do_copy_area(int area1, int area2);
void do_paste_area(int area1, int area2, const char* t);
void do_del_area(int area1, int area2);

int asio_devicecount();
const char* asio_devicename(int deviceno);
void initasio();
void asio_init(char* drivername, int samplerate);
void asio_deinit();
