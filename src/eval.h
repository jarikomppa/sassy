
extern "C"
{
#include "stb/stb_hexwave.h" // for hexwave struct
}

namespace Xbyak
{
	class CodeGenerator;
}

#define MAXROWS 128
#define MAXVAR (27*MAXROWS)

#define EPSILON 0.0001
#define FFTSIZE 2048

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

// pow(2, 3/4.0) ~ 1.6817928305074290860622509524664297900800685247135690216264521719
#define POW_2_3_4TH 1.6817928305074290860622509524664297900800685247135690216264521719

#ifndef _WIN32
#define sprintf_s snprintf
#define _strdup strdup
#endif

extern const double note_to_freq[128];

struct Op
{
	int mOpcode;
	union mArgtype
	{
		struct intpair {
			int i;
			int j;
		} i;
		double f;
		unsigned long long f64;
	} mArg;
};

struct OpContainer
{
	Op* op;
	int index;
	Op* next() { index++; return op + (index - 1); }
	Op* peek() { return op + index; }
	explicit OpContainer(Op* v) : op(v), index(0) {}
	void rewind() { index = 0; }
};


enum FUNCS
{
	FUNC_IF = 1,
	FUNC_SIN,
	FUNC_TIME,
	FUNC_GRAPH,
	FUNC_SLIDER,
	FUNC_OUT,
	FUNC_OUT2,
	FUNC_PI,
	FUNC_TRUNC,
	FUNC_FRACT,
	FUNC_MOD,
	FUNC_MIDIPOT,
	FUNC_MIDIVAL,
	FUNC_MIDINOTE,
	FUNC_MIDIVEL,
	FUNC_MIDION,
	FUNC_MIDIPITCH,
	FUNC_MIDIPROG,
	FUNC_DELAY,
	FUNC_NOISE,
	FUNC_SIN1,
	FUNC_TRIANGLE,
	FUNC_SQUARE,
	FUNC_SQUAREQ,
	FUNC_SAW,
	FUNC_SAWQ,
	FUNC_PULSE,
	FUNC_HOLD,
	FUNC_QUANTIZE,
	FUNC_SQRT,
	FUNC_LOG,
	FUNC_POW,
	FUNC_COS,
	FUNC_TAN,
	FUNC_ACOS,
	FUNC_ASIN,
	FUNC_ATAN,
	FUNC_ATAN2,
	FUNC_COSH,
	FUNC_SINH,
	FUNC_TANH,
	FUNC_EXP,
	FUNC_LOG10,
	FUNC_FLOOR,
	FUNC_ABS,
	FUNC_ADSR,
	FUNC_BUTTON,
	FUNC_TOGGLE,
	FUNC_FILTER,
	FUNC_LPF,
	FUNC_HPF,
	FUNC_BPF,
	FUNC_FFT,
	FUNC_NOTETOFREQ,
	FUNC_FREQTONOTE,
	FUNC_LATCH,
	FUNC_AREAMIN,
	FUNC_AREAMAX,
	FUNC_AVERAGE,
	FUNC_SUM,
	FUNC_SELECT,
	FUNC_SELECTV,
	FUNC_ISNAN,
	FUNC_NAN,
	FUNC_TIMEGATE,
	FUNC_NANKILL,
	FUNC_SUPERSAW,
	FUNC_SUPERSQUARE,
	FUNC_SUPERSIN,
	FUNC_SQUAREF,
	FUNC_SAWF,
	FUNC_SQUARESAW,
	FUNC_AND,
	FUNC_OR,
	FUNC_NOT,
	FUNC_XOR,
	FUNC_TRUE,
	FUNC_FALSE,
	FUNC_DEGREES,
	FUNC_RADIANS,
	FUNC_EVEN,
	FUNC_ODD,
	FUNC_SIGN,
	FUNC_BAR,
	FUNC_MIX,
	FUNC_SMOOTHSTEP,
	FUNC_SMOOTHERSTEP,
	FUNC_MIN,
	FUNC_MAX,
	FUNC_CLAMP,
	FUNC_MAP,
	FUNC_COUNT,
	FUNC_FIND,
	FUNC_FINDV,
	FUNC_PRODUCT,
	FUNC_HILIGHT,
	FUNC_REPLACE,
	FUNC_LOOKUP,
	FUNC_ROWOF,
	FUNC_COLUMNOF,
	FUNC_DT,
	FUNC_STEP,
	FUNC_ALLPASS,
	FUNC_COMB,
	FUNC_REVERB,
	FUNC_PINKNOISE,
	FUNC_BROWNNOISE,
	FUNC_BLUENOISE,
	FUNC_PARANOISE,
	FUNC_OPL1,
	FUNC_OPL2,
	FUNC_OPL3,
	FUNC_SLIDERV,
	FUNC_TOGGLEV,
	FUNC_MIDIPOTV,
	FUNC_NOTETOFREQSLOW,
	FUNC_TRIGGER,
	FUNC_TRIGGERM,
	FUNC_B_AND,
	FUNC_B_OR,
	FUNC_B_NOT,
	FUNC_B_XOR,
	FUNC_B_NAND,
	FUNC_B_NOR,
	FUNC_B_SHLEFT,
	FUNC_B_SHRIGHT,
	FUNC_B_ROTLEFT,
	FUNC_B_ROTLEFTV,
	FUNC_B_ROTRIGHT,
	FUNC_B_ROTRIGHTV,
	FUNC_B_TEST,
	FUNC_B_SET,
	FUNC_B_CLEAR,
	FUNC_HILIGHTV,
	FUNC_RGB,
	FUNC_MIDIOUT,
	FUNC_MIDIOUTPITCH,
	FUNC_MIDIOUTPOT,
	FUNC_MIDIOUTPROG,
	FUNC_MIDIOUTRAW,
	FUNC_RUBBERBAND,
	FUNC_SLIDERPOT,
	FUNC_SLIDERPOTV,
	FUNC_IN,
	FUNC_IN2,
	FUNC_DC,
	FUNC_DRUNKARDSWALK,
	FUNC_TOGGLEPOT,
	FUNC_TOGGLEPOTV,
	FUNC_CPU,
	FUNC_MIDIVALV,
	FUNC_MIDINOTEV,
	FUNC_MIDIVELV,
	FUNC_MIDIONV,
	FUNC_STEPG,
	FUNC_PLOTXY,
	FUNC_PLOTXY2,
	FUNC_HOLDL,
	FUNC_PROBE,
	FUNC_PROBEC,
	FUNC_BITCRUNCH,
	FUNC_FILTERC,
	FUNC_LPFC,
	FUNC_HPFC,
	FUNC_BPFC,
	FUNC_MIDICHANNEL,
	FUNC_MIDIVALC,
	FUNC_MIDINOTEC,
	FUNC_MIDIVELC,
	FUNC_MICIONC,
	FUNC_SCALE,
	FUNC_DISTORT,
	FUNC_CATMULLROM,
	FUNC_NOP,
	FUNC_NOP1,
	FUNC_NOP2,
	FUNC_NOP3,
	FUNC_NOP4,
	FUNC_NOP5,
	FUNC_NOP6,
	FUNC_NOP7,
	FUNC_NOP8,
	FUNC_NOP9,
	FUNC_NOP10,
	FUNC_REM,
	FUNC_IMG,
	FUNC_BARA,
	FUNC_PIE,
	FUNC_PIEA,
	FUNC_LOADWAV,
	FUNC_LOADWAVC,
	FUNC_LEN,
	FUNC_CHANNELS,
	FUNC_PLAY,
	FUNC_PLAYLOOP,
	FUNC_PLAYLOOPP,
	FUNC_SAMPLE,
	FUNC_SAMPLEFAST,
	FUNC_PLAYLOOPX,
	FUNC_GRAIN,
	FUNC_BUFFER,
	FUNC_KLATT,
	FUNC_PADSYNTH,
	FUNC_PADSYNTH22,
	FUNC_LABEL,
	FUNC_BIQUAD,
	FUNC_WRITE,
	FUNC_MIDIOUTC,
	FUNC_MIDIOUTPITCHC,
	FUNC_MIDIOUTPOTC,
	FUNC_MIDIOUTPROGC,
	FUNC_ENCODER,
	FUNC_ENCODERV,
	FUNC_AY,
	FUNC_SIDFILTER,
	FUNC_SIDVOICE,
	FUNC_SIDENVELOPE,
	FUNC_SLEWLIMIT,
	FUNC_SLEWLIMITUD,
	FUNC_SQUARESAWD,
	FUNC_AKWF,
	FUNC_BBD,
	FUNC_BBDEFF,
	FUNC_HEXWAVE,

	FUNC_LAST
};

struct FuncDef
{
	const char* mName;
	const char* mArgs;
	int mConst;
	int mSamplerateMemory;
	int mMemory;
};

static const FuncDef gFunc[] = {
{ "n/a",			"",				0,	0,	0 },
{ "if",				"CCC",			1,	0,	0 },
{ "sin",			"C",			1,	0,	0 },
{ "time",			"",				0,	0,	0 },
{ "graph",			"C",			0,	0,	(1 << 17) * sizeof(float) + sizeof(int) * 2 },
{ "slider",			"",				0,	0,	sizeof(float) + sizeof(int) },
{ "out",			"C",			0,	0,	0 },
{ "out",			"CC",			0,	0,	0 },
{ "pi",				"",				1,	0,	0 },
{ "trunc",			"C",			1,	0,	0 },
{ "fract",			"C",			1,	0,	0 },
{ "mod",			"CC",			1,	0,	0 },
{ "midipot",		"C",			0,	0,	sizeof(int) },
{ "midival",		"",				0,	0,	0 },
{ "midinote",		"",				0,	0,	0 },
{ "midivel",		"",				0,	0,	0 },
{ "midion",			"",				0,	0,	0 },
{ "midipitch",		"",				0,	0,	0 },
{ "midiprog",		"",				0,	0,	0 },
{ "delay",			"CC",			0,	sizeof(double),	sizeof(int) },
{ "noise",			"",				0,	0,	0 },
{ "sin1",			"C",			1,	0,	0 },
{ "triangle",		"C",			1,	0,	0 },
{ "square",			"C",			1,	0,	0 },
{ "squareq",		"C",			1,	0,	0 },
{ "saw",			"C",			1,	0,	0 },
{ "sawq",			"C",			1,	0,	0 },
{ "pulse",			"CC",			1,	0,	0 },
{ "hold",			"CC",			0,	0,	sizeof(double) * 2 },
{ "quantize",		"C",			1,	0,	0 },
{ "sqrt",			"C",			1,	0,	0 },
{ "log",			"C",			1,	0,	0 },
{ "pow",			"CC",			1,	0,	0 },
{ "cos",			"C",			1,	0,	0 },
{ "tan",			"C",			1,	0,	0 },
{ "acos",			"C",			1,	0,	0 },
{ "asin",			"C",			1,	0,	0 },
{ "atan",			"C",			1,	0,	0 },
{ "atan2",			"CC",			1,	0,	0 },
{ "cosh",			"C",			1,	0,	0 },
{ "sinh",			"C",			1,	0,	0 },
{ "tanh",			"C",			1,	0,	0 },
{ "exp",			"C",			1,	0,	0 },
{ "log10",			"C",			1,	0,	0 },
{ "floor",			"C",			1,	0,	0 },
{ "abs",			"C",			1,	0,	0 },
{ "adsr",			"CCCCC",		0,	0,	sizeof(double) * 2 + sizeof(int) },
{ "button",			"",				0,	0,	sizeof(int) },
{ "toggle",			"",				0,	0,	sizeof(int) + sizeof(int) },
{ "filter",			"CCCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "lpf",			"CCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "hpf",			"CCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "bpf",			"CCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "fft",			"C",			0,	0,	FFTSIZE * sizeof(float) + sizeof(int) },
{ "notetofreq",		"C",			1,	0,	0 },
{ "freqtonote",		"C",			1,	0,	0 },
{ "latch",			"CC",			0,	0,	sizeof(double) * 2 },
{ "min",			"A",			0,	0,	0 },
{ "max",			"A",			0,	0,	0 },
{ "average",		"A",			0,	0,	0 },
{ "sum",			"A",			0,	0,	0 },
{ "select",			"AC",			0,	0,	0 },
{ "selectv",		"AC",			0,	0,	0 },
{ "isnan",			"C",			1,	0,	0 },
{ "nan",			"",				1,	0,	0 },
{ "time",			"C",			0,	0,	sizeof(double) },
{ "nankill",		"C",			1,	0,	0 },
{ "supersaw",		"CCC",			1,	0,	0 },
{ "supersquare",	"CCC",			1,	0,	0 },
{ "supersin",		"CCC",			1,	0,	0 },
{ "squareq",		"CC",			1,	0,	0 },
{ "sawq",			"CC",			1,	0,	0 },
{ "squaresaw",		"CC",			1,	0,	0 },
{ "and",			"CC",			1,  0,	0 },
{ "or",				"CC",			1,  0,	0 },
{ "not",			"C",			1,  0,	0 },
{ "xor",			"CC",			1,  0,	0 },
{ "true",			"",				1,  0,	0 },
{ "false",			"",				1,  0,	0 },
{ "degrees",		"C",			1,  0,	0 },
{ "radians",		"C",			1,  0,	0 },
{ "even",			"C",			1,  0,	0 },
{ "odd",			"C",			1,  0,	0 },
{ "sign",			"C",			1,  0,	0 },
{ "bar",			"C",			0,  0,	sizeof(float) },
{ "mix",			"CCC",			1,  0,	0 },
{ "smoothstep",		"C",			1,  0,	0 },
{ "smootherstep",	"C",			1,  0,	0 },
{ "min",			"CC",			1,  0,	0 },
{ "max",			"CC",			1,  0,	0 },
{ "clamp",			"CCC",			1,  0,	0 },
{ "map",			"CCCCC",		1,  0,	0 },
{ "count",			"A",			0,	0,	0 },
{ "find",			"AC",			0,	0,	0 },
{ "findv",			"AC",			0,	0,	0 },
{ "product",		"A",			0,	0,	0 },
{ "hilight",		"CC",			0,	0,	0 },
{ "replace",		"CCC",			0,	0,	0 },
{ "lookup",			"CC",			0,	0,	0 },
{ "rowof",			"V",			1,	0,	0 },
{ "columnof",		"V",			1,	0,	0 },
{ "dt",				"",				1,  0,	0 },
{ "step",			"C",			0,  0,	sizeof(double) },
{ "allpass",		"CCC",			0,  sizeof(double),	 sizeof(int) },
{ "comb",			"CCCC",			0,  sizeof(double),	 sizeof(double) + sizeof(int) },
{ "reverb",			"CCCC",			0,  sizeof(double),  sizeof(int) * 12 }, // 1200*4*x is ok for 44100khz, so samplerate*x ought to be fine
{ "pinknoise",		"",				0,  0,	sizeof(double) * 10 + sizeof(int) * 10 },
{ "brownnoise",		"",				0,  0,	sizeof(double) * 10 + sizeof(int) * 10 },
{ "bluenoise",		"",				0,  0,	sizeof(double) * 10 + sizeof(int) * 10 },
{ "paranoise",		"CCCCCCCCCC",	0,  0,	sizeof(double) * 10 + sizeof(int) * 10 },
{ "opl1",			"C",			1,	0,	0 },
{ "opl2",			"C",			1,	0,	0 },
{ "opl3",			"C",			1,	0,	0 },
{ "slider",			"L",			0,  0,	sizeof(float) + sizeof(int) },
{ "toggle",			"L",			0,	0,	sizeof(int) + sizeof(int) },
{ "midipot",		"LC",			0,	0,	sizeof(int) },
{ "notetofreqslow",	"C",			1,	0,	0 },
{ "trigger",        "C",            0,  0,	sizeof(double) },
{ "trigger",        "CC",           0,  0,	sizeof(double) },
{ "b_and", 			"CC",			1,	0,	0 },
{ "b_or", 			"CC",			1,	0,	0 },
{ "b_not", 			"C",			1,	0,	0 },
{ "b_xor", 			"CC",			1,	0,	0 },
{ "b_nand",			"CC",			1,	0,	0 },
{ "b_nor", 			"CC",			1,	0,	0 },
{ "b_shleft",		"CC",			1,	0,	0 },
{ "b_shright",		"CC",			1,	0,	0 },
{ "b_rotleft",		"CC",			1,	0,	0 },
{ "b_rotleft",		"CCC",			1,	0,	0 },
{ "b_rotright",		"CC",			1,	0,	0 },
{ "b_rotright",		"CCC",			1,	0,	0 },
{ "b_test",			"CC",			1,	0,	0 },
{ "b_set", 			"CC",			1,	0,	0 },
{ "b_clear",		"CC",			1,	0,	0 },
{ "hilight",		"CCC", 			0,	0,	0 },
{ "rgb",            "CCC",          1,  0,	0 },
{ "midiout",        "CC", 			0,  0,	0 },
{ "midioutpitch",   "C",            0,  0,	0 },
{ "midioutpot",     "CC",           0,  0,	0 },
{ "midioutprog",    "C",            0,  0,	0 },
{ "midioutraw",     "CCCC",         0,  0,	0 },
{ "rubberband",     "CC",			0,  0,	sizeof(double) },
{ "sliderpot",      "C",            0,  0,	sizeof(float) + sizeof(int) },
{ "sliderpot",      "LC",           0,  0,	sizeof(float) + sizeof(int) },
{ "in",             "",             0,  0,	0 },
{ "in",             "C",            0,  0,	0 },
{ "dc",             "C",            0,  sizeof(double),	sizeof(double) + sizeof(int) },
{ "drunkardswalk",  "CC",           0,  0,	sizeof(double) + sizeof(int) },
{ "togglepot",      "C",            0,  0,	sizeof(int) + sizeof(int) + sizeof(double) },
{ "togglepot",      "LC",           0,  0,	sizeof(int) + sizeof(int) + sizeof(double) },
{ "cpu",            "",             0,  0,	0 },
{ "midival",		"C",			0,	0,	0 },
{ "midinote",		"C",			0,	0,	0 },
{ "midivel",		"C",			0,	0,	0 },
{ "midion",			"C",			0,	0,	0 },
{ "step",           "CC",           0,  0,	sizeof(double) },
{ "plotxy",			"C",			0,  0,	sizeof(int) + sizeof(float) * 1024 * 2 },
{ "plotxy",			"CC",			0,  0,	sizeof(int) + sizeof(float) * 1024 * 2 },
{ "hold",			"CCC",			0,	0,	sizeof(double) * 3 },
{ "probe",          "C",            0,  0,	0 },
{ "probe",          "CC",           0,  0,	0 },
{ "bitcrunch",      "CC",           1,  0,	0 },
{ "filter",			"CCCCC",		0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "lpf",			"CCCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "hpf",			"CCCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "bpf",			"CCCC",			0,	0,	(sizeof(double) * 5 + sizeof(int)) * 6 },
{ "midichannel", 	"C",			0,	0,	0 },
{ "midival",		"CC",			0,	0,	0 },
{ "midinote",		"CC",			0,	0,	0 },
{ "midivel",		"CC",			0,	0,	0 },
{ "midion",			"CC",			0,	0,	0 },
{ "scale",          "CC",           1,  0,	0 },
{ "distort",        "C",            1,  0,	0 },
{ "catmullrom",     "CCCCC",        1,  0,	0 },
{ "nop",            "",             1,  0,	0 },
{ "nop",            "C",            1,  0,	0 },
{ "nop",            "CC",           1,  0,	0 },
{ "nop",            "CCC",          1,  0,	0 },
{ "nop",            "CCCC",         1,  0,	0 },
{ "nop",            "CCCCC",        1,  0,	0 },
{ "nop",            "CCCCCC",       1,  0,	0 },
{ "nop",            "CCCCCCC",      1,  0,	0 },
{ "nop",            "CCCCCCCC",     1,  0,	0 },
{ "nop",            "CCCCCCCCC",    1,  0,	0 },
{ "nop",            "CCCCCCCCCC",   1,  0,	0 },
{ "rem",            "T",            1,  0,	0 },
{ "img",            "TCC",          0,  0,  sizeof(void*) + sizeof(double) * 2 },
{ "bar",            "A",            0,  0,  sizeof(int) * 2 },
{ "pie",            "C",            0,  0,  sizeof(double) },
{ "pie",            "A",            0,  0,  sizeof(int) * 2 },
{ "loadwav",        "T",            0,  0,  sizeof(void*) },
{ "loadwav",        "TC",           0,  0,  sizeof(void*) },
{ "len",            "C",            0,  0,  0 },
{ "channels",       "C",            0,  0,  0 },
{ "play",			"CC",           0,  0,  sizeof(int) + sizeof(double) },
{ "playloop",		"CCC",			0,	0,	sizeof(int) + sizeof(int) + sizeof(double) },
{ "playloop",		"CCCCC",		0,	0,	sizeof(int) + sizeof(int) + sizeof(double) },
{ "sample",			"CC",			0,	0,	0, },
{ "samplefast",     "CC",           0,  0,  0, },
{ "playloopx",		"CCCCC",		0,	0,	sizeof(int) + sizeof(double) * 2 },
{ "grain",          "CCCC", 		0,  0,  sizeof(int) },
{ "buffer",         "L",            0,  0,  sizeof(void*) },
{ "klatt",          "T",            0,  0,  sizeof(void*) + sizeof(int) },
{ "padsynth",       "CCCCCCCCCCC",  0,  0,  sizeof(void*) + sizeof(double) * 11 },
{ "padsynth22",     "CCCCCCCCCCC",  0,  0,  sizeof(void*) + sizeof(double) * 11 },
{ "label",			"T",			0,	0,	0, },
{ "biquad",			"CCCCCCC",		0,	0,	sizeof(double) * 4 },
{ "write",			"CCCC",			0,	0,	0, },
{ "midiout",        "CCC", 			0,  0,	0 },
{ "midioutpitch",   "CC",           0,  0,	0 },
{ "midioutpot",     "CCC",          0,  0,	0 },
{ "midioutprog",    "CC",           0,  0,	0 },
{ "encoder",		"",				0,  0,	sizeof(float) + sizeof(int) },
{ "encoder",		"L",			0,  0,	sizeof(float) + sizeof(int) },
{ "ay",             "CCCCCCCCCCC",  0,  0,  32768 }, // about 23000 actually needed
{ "sidfilter",      "CCCC",         0,  0,  sizeof(int) * 2 },
{ "sidvoice",       "CCC",          0,  0,  sizeof(int) * 8 },
{ "sidenvelope",	"CCCCC",        0,  0,  sizeof(int) * 4 },
{ "slewlimit",		"CC",			0,	0,	sizeof(double) },
{ "slewlimit",		"CCC",			0,	0,	sizeof(double) },
{ "squaresaw",		"CCC",			1,	0,	0 },
{ "akwf",           "CC",           0,  0,  0 },
{ "bbd",            "CC",           0,  0, sizeof(double) * 1024 },
{ "bbd",            "CCC",          0,  0, sizeof(double) * 1024 },
{ "hexwave",        "CCCCC",        0,  sizeof(HexWave) + 1, 0 },
};

extern int gSamplerate;

bool isoper(int c);
int precedence(Op &v);

typedef double (*jitfunc)(double *aCellval, double *aCellvalw);

void parse(const char* s, Op* bc, char *textstore);
bool lexify(Op* bc);
void opt(Op* bc);
int postfix(Op* bc); // returns number of bytes of memory needed
double compute(Op* bc, int cell, char *stringstore);
jitfunc jit(Op* bc, int cell, Xbyak::CodeGenerator &code, char*stringstore); // returns jitted code or null if failed
void jit_init(Xbyak::CodeGenerator& code);
int jit_addcode(Op* bc, int cell, Xbyak::CodeGenerator& code, char *stringstore);
jitfunc jit_finish(Xbyak::CodeGenerator& code);

bool isalpha(char c);
bool isnum(char c);

void init_symbols();
void add_symbol(const char* sym, int symtype);
int is_symbol(const char* sym);
const char* find_tab(const char* partial);
void remove_symbol(const char* sym);
void add_function(const char* sym, int symtype);
int get_function(int funcsym, const char* sig);

namespace EvalFunc
{

	double sin1(double v);
	double triangle(double v);
	double square(double v);
	double squareq(double v);
	double squaref(double v, double o);
	double saw(double v);
	double sawq(double v);
	double sawf(double v, double o);
	double pulse(double v, double d);
	double supersaw(double v, double s, double d);
	double supersquare(double v, double s, double d);
	double supersin(double v, double s, double d);
	double squaresaw(double v, double f);

	double getvar(int var);
	double gettime();
	double graph(double v, int cell, int memofs);
	double slider(double v, int cell, int memofs);
	double button(int cell, int memofs);
	double toggle(double v, int cell, int memofs);
	double output(double val);
	double output2(double val1, double val2);
	double midipot(double v, double index, int cell, int memofs);
	double midival();
	double midinote();
	double midivel();
	double midion();
	double midipitch();
	double midiprog();
	double delay(double v, double t, int cell, int memofs);
	double noise();
	double hold(double v, double t, int cell, int memofs);
	double adsr(double a, double d, double s, double r, double gate, int cell, int memofs);
	double filter(double v, double f, double r, double t, double c, int cell, int memofs);
	double fft(double v, int cell, int memofs);
	double latch(double v, double l, int cell, int memofs);
	double tablemin(double a, double b);
	double tablemax(double a, double b);
	double tableaverage(double a, double b);
	double tablesum(double a, double b);
	double tableselect(double a, double b, double v);
	double tableselectv(double a, double b, double v);
	double tablecount(double a, double b);
	double tablefind(double a, double b, double v);
	double tablefindv(double a, double b, double v);
	double tableproduct(double a, double b);
	double timegate(double v, int cell, int memofs);
	double bar(double v, int cell, int memofs);
	double pie(double v, int cell, int memofs);
	double hilight(double row, double column, double color);
	double replace(double row, double column, double value);
	double lookup(double row, double column);
	double step(double s, int cell, int memofs);
	double allpass(double v, double t, double feedback, int cell, int memofs);
	double comb(double v, double t, double damp, double feedback, int cell, int memofs);
	double reverb(double v, double roomsize, double damp, double freeze, int cell, int memofs);
	double paranoise(double oct0, double oct1, double oct2, double oct3, double oct4, double oct5, double oct6, double oct7, double oct8, double oct9, int cell, int memofs);
	double opl1(double v);
	double opl2(double v);
	double opl3(double v);
	double trigger(double v, double mode, int cell, int memofs);
	double b_and(double a, double b);
	double b_or(double a, double b);
	double b_not(double a);
	double b_xor(double a, double b);
	double b_nand(double a, double b);
	double b_nor(double a, double b);
	double b_shleft(double a, double amt);
	double b_shright(double a, double amt);
	double b_rotleft(double a, double amt, double bits);
	double b_rotright(double a, double amt, double bits);
	double b_test(double a, double bit);
	double b_set(double a, double bit);
	double b_clear(double a, double bit);
	double rgb(double r, double g, double b);
	double midiout(double note, double vel);
	double midioutpitch(double pitch);
	double midioutpot(double value, double index);
	double midioutprog(double prog);
	double midioutraw(double gate, double a, double b, double c);
	double rubberband(double val, double str, int cell, int memofs);
	double sliderpot(double v, double pot, int cell, int memofs);
	double dt();
	double in();
	double in2(double channel);
	double dc(double v, int cell, int memofs);
	double drunkardswalk(double steps, double gate, int cell, int memofs);
	double togglepot(double v, double pot, int cell, int memofs);
	double cpu();
	double midivalv(double ch);
	double midinotev(double ch);
	double midivelv(double ch);
	double midionv(double ch);
	double stepg(double s, double gate, int cell, int memofs);
	double plotxy(double a, int cell, int memofs);
	double plotxy2(double a, double b, int cell, int memofs);
	double holdl(double v, double t, double l, int cell, int memofs);
	double probe(double c, double v);
	double bitcrunch(double v, double bits);
	double midichannel(double ch);
	double midivalc(double nn, double ch);
	double midinotec(double nn, double ch);
	double midivelc(double nn, double ch);
	double midionc(double nn, double ch);
	double scale(double note, double scale);
	double distort(double v);
	double catmullrom(double t, double p0, double p1, double p2, double p3);
	double img(char* fn, double xi, double yi, int cell, int memofs);
	double bara(double a, double b, int cell, int memofs);
	double piea(double a, double b, int cell, int memofs);
	double loadwav(char* fn, int cell, int memofs);
	double loadwavc(char* fn, double channel, int cell, int memofs);
	double len(double handle);
	double channels(double handle);
	double play(double handle, double trigger, int cell, int memofs);
	double playloop(double handle, double type, double gate, int cell, int memofs);
	double playloopp(double handle, double loopstart, double loopend, double type, double gate, int cell, int memofs);
	double sample(double handle, double pos);
	double samplefast(double handle, double pos);
	double playloopx(double handle, double loopstart, double loopend, double fade, double gate, int cell, int memofs);
	double grain(double handle, double pos, double grainsize, double fade, int cell, int memofs);
	double buffer(double len, int cell, int memofs);
	double klatt(char* txt, int cell, int memofs);
	double padsynth(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs);
	double padsynth22(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs);
	double label(char* txt, int cell, int memofs);
	double biquad(double v, double a0, double a1, double a2, double b0, double b1, double b2, int cell, int memofs);
	double write(double handle, double pos, double value, double gate);
	double midioutc(double channel, double note, double vel);
	double midioutpitchc(double channel, double pitch);
	double midioutpotc(double channel, double value, double index);
	double midioutprogc(double channel, double prog);
	double encoder(double v, int cell, int memofs);
	double ay(double tone1, double tone2, double tone3, double noise, double vol1, double vol2, double vol3, double envelope, double envshape, double mixer, double envgate, int cell, int memofs);
	double sidfilter(double v, double mode, double resonance, double frequency, int cell, int memofs);
	double sidvoice(double freq, double pulseduty, double mode, int cell, int memofs);
	double sidenvelope(double attack, double decay, double sustain, double release, double gate, int cell, int memofs);
	double slewlimit(double v, double max, int cell, int memofs);
	double slewlimitud(double v, double maxu, double maxd, int cell, int memofs);
	double squaresawd(double v, double f, double d);
	double akwf(double v, double s);
	double bbd(double v, double count, int cell, int memofs);
	double bbdeff(double v, double count, double eff, int cell, int memofs);
	double hexwave(double reflect, double peaktime, double halfheight, double zerowait, double freq, int cell, int memofs);
};
