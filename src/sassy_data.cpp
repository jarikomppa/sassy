#include "sassy.h"

int gSamplerate = 44100;

SassyConfig gConfig;

Celldata* gCelldata;

ScopeData* gScope;

SDL_mutex* gAudioMutex;

volatile int gResourceLoaderRunning = 0;


ffft::FFTReal<float> gFft_object(FFTSIZE);
ffft::FFTReal<float> gFft_object_16(16*2);
ffft::FFTReal<float> gFft_object_32(32*2);
ffft::FFTReal<float> gFft_object_64(64*2);
ffft::FFTReal<float> gFft_object_128(128*2);
ffft::FFTReal<float> gFft_object_256(256*2);
ffft::FFTReal<float> gFft_object_512(512*2);
ffft::FFTReal<float> gFft_object_1024(1024*2);
ffft::FFTReal<float> gFft_object_2048(2048*2);
ffft::FFTReal<float> gFft_object_4096(4096*2);
ffft::FFTReal<float> gFft_object_8192(8192*2);
ffft::FFTReal<float> gFft_object_16384(16384*2);
ffft::FFTReal<float> gFft_object_32768(32768*2);
ffft::FFTReal<float> gFft_object_65536(65536*2);

WavResource* gWavHandle[MAX_HANDLES];

RtMidiIn* gRtmidi_in = NULL;
RtMidiOut* gRtmidi_out = NULL;
double gMidiCredits = 0;

int gCopyofs = 0;

int gEditorcell = 0;
int gArea1 = -1, gArea2 = -1;

bool gMuted = false;

double* gCellvalue;
double* gCellvaluew;

bool gLaunchpadMode = false;

bool gWasdown[512];

double gTime = 0;
float gOutputSample[2] = { 0, 0 };
float gCPU = 0;
float gCPU_peak = 0;
float gCPU_avg_sum = 0;
#define AVG_SAMPLES 200
float gCPU_avg_data[AVG_SAMPLES];
int gCPU_avg_idex = 0;
WELL512 gRandom;

double gMidi_pot[128];
int gMidi_pot_changed[128];
int gMidi_pot_cell[128];
double gMidi_prog = 0;
double gMidi_pitch = 0.5f;
int gMidi_noteon = 0;
double gMidi_noteval = 0;
double gMidi_note = 0x3c;
double gMidi_notevel = 0;

int gMidi_poly_notetoindex[128 * 16];
int gMidi_poly_channel[128];
int gMidi_poly_noteon[128];
double gMidi_poly_noteval[128];
double gMidi_poly_note[128];
double gMidi_poly_notevel[128];

int gMidi_perchan_notetoindex[128 * 16];
int gMidi_perchan_noteon[128 * 16];
double gMidi_perchan_note[128 * 16];
double gMidi_perchan_noteval[128 * 16];
double gMidi_perchan_notevel[128 * 16];

unsigned char gOutmidivel[128 * 16];
double gOutmidiupdate[128 * 16];
int gOutmidipitch[16] = { 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192 };
int gOutmidipot[128 * 16];
int gOutmidiprog[16] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

int gActiveMidiout[128 * 16];
int gActiveMidiouts = 0;

SDL_AudioSpec gActiveAudioSpec, gActiveCaptureSpec;
SDL_AudioDeviceID gAudioDeviceID, gCaptureDeviceID;

SRC_STATE* gCaptureResampler = 0;

bool gShowHelpWindow = false;
bool gShowScopeWindow = false;
bool gShowKeyboardWindow = false;
bool gShowMidiPlayerWindow = false;

char gMidimessage[256] = { 'n', '/', 'a', 0 };


float gProbe[4];

const char* gNotestr[128] =
{
    "C0-1","C#-1","D-1","D#-1","E-1","F-1","F#-1","G-1","G#-1","A-1","A#-1","B-1",
    "C0","C#0","D0","D#0","E0","F0","F#0","G0","G#0","A0","A#0","B0",
    "C1","C#1","D1","D#1","E1","F1","F#1","G1","G#1","A1","A#1","B1",
    "C2","C#2","D2","D#2","E2","F2","F#2","G2","G#2","A2","A#2","B2",
    "C3","C#3","D3","D#3","E3","F3","F#3","G3","G#3","A3","A#3","B3",
    "C4","C#4","D4","D#4","E4","F4","F#4","G4","G#4","A4","A#4","B4",
    "C5","C#5","D5","D#5","E5","F5","F#5","G5","G#5","A5","A#5","B5",
    "C6","C#6","D6","D#6","E6","F6","F#6","G6","G#6","A6","A#6","B6",
    "C7","C#7","D7","D#7","E7","F7","F#7","G7","G#7","A7","A#7","B7",
    "C8","C#8","D8","D#8","E8","F8","F#8","G8","G#8","A8","A#8","B8",
    "C9","C#9","D9","D#9","E9","F9","F#9","G9"
};

Op gBC[1024];

int gActivecell[MAXVAR];
int gActivecells = 0;

float gInputSample[2] = { 0, 0 }; 
float gCaptureData[CAPTURE_LEN * 2];
int gCaptureWrite = 0;
int gCaptureRead = 0;

double gCPUTimePerSample;
