#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"
#include "xbyak/xbyak.h"
#include "well512.h"

using namespace EvalFunc;

int gSamplerate = 8000;

namespace EvalFunc
{

	double getvar(int var)
	{
		return var;
	}

	double gettime()
	{
		return 242;
	}

	double graph(double v, int cell, int memofs)
	{
		return v;
	}

	double slider(double v, int cell, int memofs)
	{
		return v;
	}

	double encoder(double v, int cell, int memofs)
	{
		return v;
	}

	double output(double val)
	{
		return val;
	}

	double output2(double val, double)
	{
		return val;
	}

	double midipot(double v, double index, int cell, int memofs)
	{
		return v * 10 + index;
	}

	double midival()
	{
		return 440;
	}

	double midinote()
	{
		return 23;
	}

	double midivel()
	{
		return 1;
	}

	double midion()
	{
		return 0;
	}

	double midipitch()
	{
		return 0.5f;
	}

	double midiprog()
	{
		return 3;
	}

	double delay(double v, double t, int cell, int memofs)
	{
		return v * t;
	}

	double noise()
	{
		return 4; // xkcd 221
	}

	double hold(double v, double t, int cell, int memofs)
	{
		return v * t;
	}

	double holdl(double v, double t, double l, int cell, int memofs)
	{
		return v * t - l;
	}

	double adsr(double a, double d, double s, double r, double gate, int cell, int memofs)
	{
		return a + d * s - r * gate;
	}

	double button(int cell, int memofs)
	{
		return 1;
	}

	double toggle(double v, int cell, int memofs)
	{
		return v;
	}

	double filter(double v, double f, double r, double t, double c, int cell, int memofs)
	{
		return v + f * r - t + c - 1;
	}

	double fft(double v, int cell, int memofs)
	{
		return v;
	}

	double latch(double v, double l, int cell, int memofs)
	{
		return v + l;
	}

	double tablemin(double a, double b)
	{
		return a + b;
	}

	double tablemax(double a, double b)
	{
		return a + b;
	}

	double tableaverage(double a, double b)
	{
		return a + b;
	}

	double tablesum(double a, double b)
	{
		return a + b;
	}

	double tableselect(double a, double b, double v)
	{
		return (a + b) * v;
	}

	double tableselectv(double a, double b, double v)
	{
		return (a + b) * v;
	}

	double timegate(double v, int cell, int memofs)
	{
		return 3;
	}

	double bar(double v, int cell, int memofs)
	{
		return v;
	}

	double pie(double v, int cell, int memofs)
	{
		return v;
	}

	double tablecount(double a, double b)
	{
		return a + b;
	}

	double tablefind(double a, double b, double v)
	{
		return a + b + v;
	}
	double tablefindv(double a, double b, double v)
	{
		return a + b + v;
	}
	double tableproduct(double a, double b)
	{
		return a + b;
	}

	double hilight(double row, double column, double color)
	{
		return row + 100 * column + 100000 * color;
	}

	double replace(double row, double column, double value)
	{
		return row + column + value;
	}

	double lookup(double row, double column)
	{
		return row + column;
	}

	double step(double s, int cell, int memofs)
	{
		return s;
	}

	double allpass(double v, double t, double feedback, int cell, int memofs)
	{
		return v + t + feedback;
	}

	double comb(double v, double t, double damp, double feedback, int cell, int memofs)
	{
		return v + t + damp + feedback;
	}

	double reverb(double v, double roomsize, double damp, double freeze, int cell, int memofs)
	{
		return v + roomsize + damp + freeze;
	}

	double paranoise(double oct0, double oct1, double oct2, double oct3, double oct4, double oct5, double oct6, double oct7, double oct8, double oct9, int cell, int memofs)
	{
		return
			oct0 +
			oct1 * 2 +
			oct2 * 3 +
			oct3 * 5 +
			oct4 * 7 +
			oct5 * 11 +
			oct6 * 13 +
			oct7 * 17 +
			oct8 * 23 +
			oct9 * 27;
	}

	double trigger(double v, double mode, int cell, int memofs)
	{
		return v * 10 + mode;
	}

	double midiout(double note, double vel)
	{
		return note + vel;
	}

	double midioutpitch(double pitch)
	{
		return pitch;
	}

	double midioutpot(double value, double index)
	{
		return value + index;
	}

	double midioutprog(double prog)
	{
		return prog;
	}

	double midioutraw(double gate, double a, double b, double c)
	{
		return a + b + c;
	}

	double rubberband(double val, double str, int cell, int memofs)
	{
		return val + str;
	}

	double sliderpot(double v, double pot, int cell, int memofs)
	{
		return v * 7 + pot;
	}

	double dt()
	{
		return 1 / 44100.0;
	}

	double in()
	{
		return 0.2;
	}

	double in2(double channel)
	{
		return channel * 0.1 + 0.1;
	}

	double dc(double v, int cell, int memofs)
	{
		return v / 2;
	}

	double drunkardswalk(double steps, double gate, int cell, int memofs)
	{
		return gate + steps * 4;
	}

	double togglepot(double v, double pot, int cell, int memofs)
	{
		return v - pot;
	}

	double cpu()
	{
		return 2;
	}

	double midivalv(double ch)
	{
		return ch + 3;
	}

	double midinotev(double ch)
	{
		return ch + 3;
	}

	double midivelv(double ch)
	{
		return ch + 3;
	}

	double midionv(double ch)
	{
		return ch + 3;
	}

	double stepg(double s, double gate, int cell, int memofs)
	{
		return s - gate;
	}

	double plotxy(double a, int cell, int memofs)
	{
		return 0;
	}

	double plotxy2(double a, double b, int cell, int memofs)
	{
		return b;
	}

	double probe(double c, double v)
	{
		return c - v;
	}

	double midichannel(double ch)
	{
		return ch;
	}

	double midivalc(double nn, double ch)
	{
		return nn + ch;
	}

	double midinotec(double nn, double ch)
	{
		return nn + ch;
	}

	double midivelc(double nn, double ch)
	{
		return nn + ch;
	}

	double midionc(double nn, double ch)
	{
		return nn + ch;
	}

	double img(char* fn, double xi, double yi, int cell, int memofs)
	{
		return xi * (strcmp(fn, "foo") == 0);
	}

	double bara(double a, double b, int cell, int memofs)
	{
		return b;
	}

	double piea(double a, double b, int cell, int memofs)
	{
		return b;
	}

	double loadwav(char* fn, int cell, int memofs)
	{
		return strcmp(fn, "foo");
	}

	double loadwavc(char* fn, double channel, int cell, int memofs)
	{
		return strcmp(fn, "foo") + channel;
	}

	double len(double v)
	{
		return v;
	}

	double channels(double v)
	{
		return v;
	}

	double play(double handle, double trigger, int cell, int memofs)
	{
		return trigger;
	}

	double playloop(double handle, double type, double gate, int cell, int memofs)
	{
		return gate;
	}

	double playloopp(double handle, double loopstart, double loopend, double type, double gate, int cell, int memofs)
	{
		return handle;
	}

	double playloopx(double handle, double loopstart, double loopend, double type, double gate, int cell, int memofs)
	{
		return handle;
	}

	double sample(double handle, double pos)
	{
		return pos;
	}

	double samplefast(double handle, double pos)
	{
		return pos;
	}

	double grain(double handle, double pos, double grainsize, double fade, int cell, int memofs)
	{
		return pos;
	}

	double buffer(double len, int cell, int memofs)
	{
		return len;
	}

	double klatt(char* txt, int cell, int memofs)
	{
		return strcmp(txt, "bar");
	}

	double padsynth(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs)
	{
		return bw +
			2 * bws +
			4 * relf +
			8 * a1 +
			16 * a1 +
			32 * a2 +
			64 * a3 +
			128 * a4 +
			256 * a5 +
			512 * a6 +
			1024 * a7 +
			2047 * a8;
	}

	double padsynth22(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs)
	{
		return 22 +
			bw +
			2 * bws +
			4 * relf +
			8 * a1 +
			16 * a1 +
			32 * a2 +
			64 * a3 +
			128 * a4 +
			256 * a5 +
			512 * a6 +
			1024 * a7 +
			2047 * a8;
	}

	double ay(double tone1, double tone2, double tone3, double noise, double vol1, double vol2, double vol3, double envelope, double envshape, double mixer, double envgate, int cell, int memofs)
	{
		return tone2;
	}

	double label(char* txt, int cell, int memofs)
	{
		return strcmp(txt, "bar");
	}

	double biquad(double v, double a0, double a1, double a2, double b0, double b1, double b2, int cell, int memofs)
	{
		return a0;
	}

	double write(double handle, double pos, double value, double gate)
	{
		return pos;
	}

	double midioutc(double channel, double note, double vel)
	{
		return channel;
	}

	double midioutpitchc(double channel, double pitch)
	{
		return channel;
	}
	
	double midioutpotc(double channel, double value, double index)
	{
		return channel;
	}

	double midioutprogc(double channel, double prog)
	{
		return channel;
	}

	double sidfilter(double v, double mode, double resonance, double frequency, int cell, int memofs)
	{
		return mode;
	}

	double sidvoice(double freq, double pulseduty, double mode, int cell, int memofs)
	{
		return pulseduty;
	}

	double sidenvelope(double attack, double decay, double sustain, double release, double gate, int cell, int memofs)
	{
		return decay;
	}

	double slewlimit(double v, double max, int cell, int memofs)
	{
		return v;
	}

	double slewlimitud(double v, double maxu, double maxd, int cell, int memofs)
	{
		return v;
	}

	double akwf(double v, double s)
	{
		return s;
	}

	double bbd(double v, double count, int cell, int memofs)
	{
		return count;
	}

	double bbdeff(double v, double count, double eff, int cell, int memofs)
	{
		return count;
	}

	double hexwave(double reflect, double peaktime, double halfheight, double zerowait, double freq, int cell, int memofs)
	{
		return zerowait;
	}

}; // namespace EvalFunc

void printtokens(Op* bc, char *logstring)
{
	char temp[2048];
	while (bc->mOpcode)
	{
		if (bc->mOpcode == 'C')
		{
			sprintf(temp, "%5.3f ", bc->mArg.f);
			strcat(logstring, temp);
		} else
		if (bc->mOpcode == 'V')
		{
			sprintf(temp, "%c(%d) ", bc->mOpcode, bc->mArg.i.i);
			strcat(logstring, temp);
		}
		else	
		if (bc->mOpcode == 'T')
		{
			sprintf(temp, "%c(%d) ", bc->mOpcode, bc->mArg.i.i);
			strcat(logstring, temp);
		}
		else
		if (bc->mOpcode == 'F')
		{
			sprintf(temp, "%s#%d ", gFunc[bc->mArg.i.i].mName, bc->mArg.i.i);
			strcat(logstring, temp);
		}
		else
		if (bc->mOpcode == 'A')
		{
			sprintf(temp, "A(%d,%d) ", bc->mArg.i.i, bc->mArg.i.j);
			strcat(logstring, temp);
		}
		else
		{
			sprintf(temp, "%c ", bc->mOpcode);
			strcat(logstring, temp);
		}
		bc++;
	}
	strcat(logstring, "\n");
}

Xbyak::CodeGenerator code(4096);

double cellvals[1024];

double eval(const char* s, int do_opt, int do_jit, int just_validate = 0)
{
	for (int i = 0; i < 1024; i++)
		cellvals[i] = i + 1;
	char logstring[8192];
	char stringstore[1024];
	char temp[2048];
	Op bc[1024];
	//printf("%s, opt:%d, jit:%d\n", s, do_opt, do_jit);
	logstring[0] = 0;	
	sprintf(temp, "eval:    \"%s\"\n", s);
	strcat(logstring, temp);	
	parse(s, bc, stringstore);
	sprintf(temp, "Parsed: "); strcat(logstring, temp); printtokens(bc, logstring);
	if (lexify(bc))
	{
		sprintf(temp, "Lexed:  "); strcat(logstring, temp); printtokens(bc, logstring);
		printf("\n%s\nDid not lex.\n", logstring);
		return nanf("0");
	}
	sprintf(temp, "Lexed:  "); strcat(logstring, temp); printtokens(bc, logstring);
	if (do_opt)
	{
		opt(bc);
		sprintf(temp, "Opt'd:  "); strcat(logstring, temp); printtokens(bc, logstring);
	}
	postfix(bc);
	sprintf(temp, "Postfix:"); strcat(logstring, temp); printtokens(bc, logstring);
	double res = compute(bc, 0, stringstore);
	if (do_jit)
	{
		jitfunc jf = jit(bc, 42, code, stringstore);
		sprintf(temp, "Jitted:  %d bytes\n", (int)code.getSize());
		strcat(logstring, temp);
		if (jf == 0)
		{
			logstring[0] = 0;
			sprintf(logstring, "\"%s\" did not jit", s);
			//strcat(logstring, "Did not jit\n");
			res = nan("0");
		}
		else
		{
			double jres = jf(cellvals, cellvals);
			bool nanres = isnan(res) != isnan(jres);
			double magnitude = std::max(abs(jres), abs(res));
			bool realres = abs(jres - res) > magnitude * 0.0001;
			if (isnan(res)) realres = false;
				
			if (nanres || realres)
			{
				sprintf(temp, "Jit result does not match compute: %5.3f, expected %5.3f\n", jres, res); strcat(logstring, temp);
				res = nan("0");
				if (just_validate)
				{
					printf("\n%s\n", logstring);
					return 1;
				}
			}
			else
			{
				if (just_validate) return 0;
			}
		}
	}
	if (just_validate) return 0;
	if (res != res) printf("\n%s\n", logstring);
	return res;
}

std::string func2call(int func)
{
	std::string s = gFunc[func].mName;
	s += '(';
	const char* a = gFunc[func].mArgs;
	bool first = true;
	while (*a)
	{
		if (!first)
		{
			s += ',';
		}
		else
		{
			first = false;
		}
		s += *a;
		a++;
	}
	s += ')';
	return s;
}

WELL512 gRandom;

int fuzz()
{
	std::string s;
	// - pick a random function	
	// - build a func(v,v,v) sig
	s = func2call(gRandom.genrand_int31() % (FUNC_LAST - 1) + 1);
	// - replace v:s randomly with functions or values
	bool changed = true;
	int maxfunc = 20;
	while (changed)
	{
		changed = false;
		std::string d;
		for (int i = 0; i < s.length(); i++)
		{
			if (s[i] == 'C' && maxfunc)
			{
				if ((gRandom.genrand_int31() % 10) > 1)
				{
					d += func2call(gRandom.genrand_int31() % (FUNC_LAST - 1) + 1);
				}
				else
				{
					int braces = gRandom.genrand_int31() & 1;
					if (braces)
						d += "(";
					d += "C";
					switch (gRandom.genrand_int31() % 4)
					{
					case 0: d += "+"; break;
					case 1: d += "-"; break;
					case 2: d += "*"; break;
					case 3: d += "/"; break;
					}
					d += "C";
					if (braces)
						d += ")";
				}
				changed = true;
				maxfunc--;
			}
			else
			if (s[i] == 'L' || (s[i] == 'C' && !maxfunc))
			{
				char temp[16];
				sprintf(temp, "%3.5f", gRandom.genrand_int31() * 0.001f);
				d += temp;
				changed = true;
			}
			else
			if (s[i] == 'V')
			{
				char temp[16];
				sprintf(temp, "%c%d", 'a'+ gRandom.genrand_int31() %26, (gRandom.genrand_int31() %32) + 1);
				d += temp;
				changed = true;
			}
			else
			if (s[i] == 'A')
			{
				char temp[16];
				sprintf(temp, "%c%d:%c%d", 'a' + gRandom.genrand_int31() % 26, (gRandom.genrand_int31() % 32) + 1, 'a' + gRandom.genrand_int31() % 26, (gRandom.genrand_int31() % 32) + 1);
				d += temp;
				changed = true;
			}
			else
			if (s[i] == 'T')
			{
				d += "\"foobar\"";
				changed = true;
			}
			else
			{
			d += s[i];
			}
		}
		s = d;
	}
	// - compile, jit, run
	printf("\n%s\n", s.c_str());
	return (int)eval(s.c_str(), 1, 1, 1);
}


int main(int parc, char** pars)
{
	int testcount = 0;
	int errors = 0;
	init_symbols();

#if 0 // analyze
	eval("a1+", 0, 1, 1);
	return 0;
#endif

#if 1  // test set
#define RUNTEST(A, B, C, D) { testcount++; double v = eval(A, C, D); if (v != v || fabs(v-(double)(B)) > 0.0001) { errors++; printf("expected %3.5f, got %3.5f:\"%s\"\n", (double)B, v, A); } }
#define TEST(A, B) RUNTEST(A, B, 0, 0) RUNTEST(A, B, 1, 0) RUNTEST(A, B, 0, 1)
	TEST("0", 0);
	TEST("1", 1);
	TEST("0.5", 0.5);
	TEST(" 1   ", 1);
	TEST("  1  + \t1", 2)
	TEST("1*2", 2);
	TEST("-1", -1);
	TEST("1*-1", -1);
	TEST("1*(-1)", -1);
	TEST("1-1", 0);
	TEST("(1)-1", 0);
	TEST("1-(1)", 0);
	TEST("(1)-(1)", 0);
	TEST("-(1)", -(1));
	TEST("-(-(-(-1)))", -(-(-(-1))));
	TEST("$A1", 1);
	TEST("A$1", 1);
	TEST("$A$1", 1);
	TEST("c5-d5", -1);
	TEST("1--1", 2);
	TEST("1+2*3", 7);
	TEST("1*2+3", 5);
	TEST("(1+2)*3", 9);
	TEST("1+(2*3)", 7);
	TEST("1+(2*(3+1))", 9);
	TEST("((1+2)*3)+1", 10);
	TEST("(((1)+(((2)))*(3)))", 7);
	TEST("sin(3)", sin(3));
	TEST("sin(-1)", sin(-1));
	TEST("sin(1+2)", sin(1 + 2));
	TEST("sin(2*(3+1))", sin(2 * (3 + 1)));
	TEST(" sin( 2 +sin(1*(3+2)))-1", sin(2 + sin(1 * 3 + 2)) - 1);
	TEST("1 > 2", 0);
	TEST("2 > 1", 1);
	TEST("1 < 2", 1);
	TEST("2 < 1", 0);
	TEST("1 = 2", 0);
	TEST("1 = 1", 1);
	TEST("(6*2) = (3 * 4)", 1);
	TEST("1=2", 0);
	TEST("A1",1);
	TEST("-P32", -853);
	TEST("2*SIN(3*4)*5", 2 * sin(3 * 4) * 5);
	TEST("IF(0;1;2)", 2);
	TEST("if(3>2,9*(3*2),32)", 9 * (3 * 2));
	TEST("if(3<2,42,(2*(4*2))*2)", 32);
	TEST("time()",gettime());
	TEST("2*TIME()", 2*gettime());
	TEST("1+(((SLIDER(1)*2)*(5+3)/(2-1)))", 1 + (((1) * 2)*(5 + 3) / (2 - 1)));
	TEST("out(667)", 667);
	TEST("sin(pi())", sin(3.1415926535897932384626433832795f));
	TEST("trunc(2.5)", 2);
	TEST("fract(2.5)", 0.5);
	TEST("mod(17,10)", 7);
	TEST("midipot(7)", midipot(0, 7, 0, 0));
	TEST("midipot(3,7)", midipot(3, 7, 0, 0));
	TEST("midival()", 440);
	TEST("midinote()", 23);
	TEST("midivel()", 1);
	TEST("midion()", 0);
	TEST("midipitch()", 0.5);
	TEST("midiprog()", 3);
	TEST("delay(32, 0.5)", 32 * 0.5f);
	TEST("sin1(7.3)", sin1(7.3f));
	TEST("triangle(7.3)", triangle(7.3f));
	TEST("square(7.3)", square(7.3f));
	TEST("squareq(7.3)", squareq(7.3f));
	TEST("saw(7.3)", saw(7.3f));
	TEST("sawq(7.3)", sawq(7.3f));
	TEST("pulse(7.3,0.2)", pulse(7.3f, 0.2f));
	TEST("pulse(7.3,0.4)", pulse(7.3f, 0.4f));
	TEST("hold(2,3)", 6);
	TEST("quantize(440)", 440);
	TEST("sqrt(7)", sqrt(7));
	TEST("log(7)", log(7));
	TEST("pow(3,5)", pow(3, 5));
	TEST("cos(1.3)", cos(1.3));
	TEST("tan(0.7)", tan(0.7));
	TEST("acos(0.7)", acos(0.7));
	TEST("asin(0.7)", asin(0.7));
	TEST("atan(0.7)", atan(0.7));
	TEST("atan2(0.7,0.3)", atan2(0.7, 0.3));
	TEST("cosh(0.7)", cosh(0.7));
	TEST("sinh(0.7)", sinh(0.7));
	TEST("tanh(0.7)", tanh(0.7));
	TEST("exp(0.7)", exp(0.7));
	TEST("log10(0.7)", log10(0.7));
	TEST("floor(3.7)", floor(3.7));
	TEST("abs(-3)", 3);
	TEST("time()-5", gettime() - 5);
	TEST("adsr(2,3,4,5,6)", adsr(2, 3, 4, 5, 6, 0, 0));
	TEST("pow(adsr(2,3,4,5,6),2)", pow(adsr(2, 3, 4, 5, 6, 0, 0),2));
	TEST("button() + toggle() + toggle(3)", button(0, 0) + toggle(0, 0, 0) + toggle(3, 0, 0));
	TEST("filter(1,2,3,4)", filter(1, 2, 3, 4, 1, 0, 0));
	TEST("lpf(3,1,4)", filter(3, 1, 4, 0, 1, 0, 0));
	TEST("hpf(5,4,3)", filter(5, 4, 3, 1, 1, 0, 0));
	TEST("bpf(7,9,1)", filter(7, 9, 1, 2, 1, 0, 0));
	TEST("fft(42)", 42);
	TEST("out(3,4)", 3);
	TEST("notetofreq(69)", 440);
	TEST("freqtonote(440)", 69);
	TEST("freqtonote(notetofreq(24))", 24);
	TEST("latch(3,4)", 3 + 4);
	TEST("min(a3:c6)", 55+138);
	TEST("max(a3:c6)", 55 + 138);
	TEST("average(a3:c6)", 55 + 138);
	TEST("sum(a3:c6)", 55 + 138);
	TEST("select(a3:c6, 3)", (55 + 138) * 3);
	TEST("selectv(a3:c6, 3)", (55 + 138) * 3);
	TEST("isnan(6)", 0);
	TEST("isnan(nan())", 1);
	TEST("time(1)", 3);
	TEST("nankill(nan())", 0);
	TEST("supersaw(1,1,1)", -1.5);
	TEST("supersquare(1,1,1)", -1.5);
	TEST("supersin(1,1,1)", 0);
	TEST("sawq(1.3,14)", sawq(1.3));
	TEST("squareq(1.3,10)", squareq(1.3));
	TEST("squaresaw(1.3,0.5)", squaresaw(1.3, 0.5));
	TEST("and(1,1)+and(1,0)+and(0,1)+and(0,0)", 1);
	TEST("or(1,1)+or(1,0)+or(0,1)+or(0,0)", 3);
	TEST("not(0)", 1);
	TEST("xor(1,1)+xor(1,0)+xor(0,1)+xor(0,0)", 2);
	TEST("true()", 1);
	TEST("false()", 0);
	TEST("degrees(10)", 1800/M_PI);
	TEST("radians(10)", M_PI/18);
	TEST("even(10)", 1);
	TEST("odd(10)", 0);
	TEST("sign(10)", 1);
	TEST("bar(4)", 4);
	TEST("mix(0.5,2,4)", 3);
	TEST("smoothstep(0.1)", 0.1*0.1*(3-2*0.1));
	TEST("smootherstep(0.1)", 0.1*0.1*0.1*(0.1*(0.1*6-15)+10));
	TEST("min(2,7)", 2);
	TEST("max(2,-7)", 2);
	TEST("clamp(107,27,93)", 93);
	TEST("map(10,0,100,10,110)", 20);
	TEST("count(a1:c9)", 220);
	TEST("find(a1:c9,4)", 224);
	TEST("findv(a1:c9,4)", 224);
	TEST("product(a1:c9)", 220);
	TEST("hilight(3,4)", 165788100403);
	TEST("replace(3,4,5)", 3 + 4 + 5);
	TEST("lookup(3,4)", 3 + 4);
	RUNTEST("rowof(c15)", 15, 1, 0); // only in opt
	RUNTEST("columnof(c15)", 3, 1, 0); // only in opt
	TEST("dt()", 1.0 / 44100);
	TEST("step(123)", 123);
	TEST("allpass(1,2,3)", 1 + 2 + 3);
	TEST("comb(2,3,4,5)", 2 + 3 + 4 + 5);
	TEST("reverb(4,5,6,7)", 4 + 5 + 6 + 7);
	TEST("bluenoise()", 839);
	TEST("pinknoise()", 109);
	TEST("brownnoise()", 360);
	TEST("paranoise(2,1,4,3,6,5,8,7,10,9)", 824);
	TEST("floor(a3)", 55);
	TEST("notetofreqslow(69)", 440);
	TEST("trigger(3)", 30);
	TEST("trigger(3,2)", 32);
	TEST("b_and(2,3)", 2);
	TEST("b_or(2,3)", 3);
	TEST("b_xor(2,3)", 1);
	TEST("b_not(2)", 0xffffffff - 2);
	TEST("b_nand(2,3)", 0xffffffff - 2);
	TEST("b_nor(2,3)", 0xffffffff - 3);
	TEST("b_shleft(16,2)", 16 * 4);
	TEST("b_shright(16,2)", 16 / 4);
	TEST("b_rotleft(1,4)", (1 << 4));
	TEST("b_rotleft(1,4,3)", (1 << 1));
	TEST("b_rotright(1,4)", (1 << 28));
	TEST("b_rotright(1,4,3)", (1 << 2));
	TEST("b_test(7,4)+b_test(7,2)", 1);
	TEST("b_set(7,3)", 15);
	TEST("b_clear(7,2)", 3);
	TEST("hilight(1,2,3)", hilight(1,2,3));
	TEST("rgb(1,1,0)", 0xffff00);
	TEST("midiout(60,0.5)", 60.5);
	TEST("midioutpitch(0.2)", 0.2);
	TEST("midioutpot(1,3)", midioutpot(1, 3));
	TEST("midioutprog(7)", 7);
	TEST("midioutraw(1,2,3,4)", 2 + 3 + 4);
	TEST("rubberband(1,3)", 1 + 3);
	TEST("sliderpot(3)", 3);
	TEST("sliderpot(1,3)", 10);
	TEST("in()", 0.2);
	TEST("in(2)", 0.3);
	TEST("dc(4)", 2);
	TEST("drunkardswalk(10,2)", 42);
	TEST("togglepot(1)", -1);
	TEST("togglepot(1,1)", 0);
	TEST("cpu()", 2);
	TEST("midival(3)", 6);
	TEST("midivel(2)", 5);
	TEST("midinote(7)", 10);
	TEST("midion(6)", 9);
	TEST("step(4,4)", 0);
	TEST("plotxy(2)", 0);
	TEST("plotxy(1,2)", 2);
	TEST("hold(2,3,4)", 2 * 3 - 4);
	TEST("probe(5)", -5);
	TEST("probe(3,3)", 0);
	TEST("bitcrunch(0.7329,3.7)", bitcrunch(0.7329, 3.7));
	TEST("filter(1,2,3,4,3)", filter(1, 2, 3, 4, 3, 0, 0));
	TEST("lpf(3,1,4,3)", filter(3, 1, 4, 0, 3, 0, 0));
	TEST("hpf(5,4,3,3)", filter(5, 4, 3, 1, 3, 0, 0));
	TEST("bpf(7,9,1,3)", filter(7, 9, 1, 2, 3, 0, 0));
	TEST("midichannel(2)", 2);
	TEST("midival(1,2)", 3);
	TEST("midinote(2,3)", 5);
	TEST("midivel(3,4)", 7);
	TEST("midion(4,5)", 9);
	TEST("scale(60,3)", 60);
	TEST("distort(0)", 0);
	TEST("catmullrom(0.2,1,2,3,4)", catmullrom(0.2, 1, 2, 3, 4));
	RUNTEST("nop()", 0, 1, 0);
	RUNTEST("nop(1)", 0, 1, 0);
	RUNTEST("nop(1,2)", 0, 1, 0);
	RUNTEST("nop(1,2,3)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5,6)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5,6,7)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5,6,7,8)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5,6,7,8,9)", 0, 1, 0);
	RUNTEST("nop(1,2,3,4,5,6,7,8,9,0)", 0, 1, 0);
	RUNTEST("rem(\"well paint me blue and call me daisy\")", 0, 1, 0);
	RUNTEST("rem(\"one\")+rem(\"two\")+rem(\"three\")+rem(\"four\")", 0, 1, 0);
	TEST("img(\"foo\",5,3)", 5);
	TEST("bar(e3:a1)", 1);
	TEST("pie(3)", 3);
	TEST("pie(e3:a1)", 1);
	TEST("loadwav(\"bar\")", -1);
	TEST("loadwav(\"bar\",2)", 1);
	TEST("len(13113)", 13113);
	TEST("channels(31131)", 31131);
	TEST("play(1,2)", 2);
	TEST("playloop(1,2,3)", 3);
	TEST("playloop(1,2,3,4,5)", 1);
	TEST("sample(1,2)", 2);
	TEST("samplefast(3,4)", 4);
	TEST("playloopx(1,2,3,4,5)", 1);
	TEST("grain(1,2,3,4)", 2);
	TEST("buffer(1)", 1);
	TEST("klatt(\"bar\")", 0);
	TEST("padsynth(1,2,3,4,5,6,7,8,9,10,11)", padsynth(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,0,0));
	TEST("padsynth22(1,2,3,4,5,6,7,8,9,10,11)", padsynth22(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0));
	TEST("label(\"bar\")", 0);
	TEST("biquad(1,2,3,4,5,6,7)", 2);
	TEST("write(1,2,3,4)", 2);
	TEST("midiout(1,60,0.5)", 1);
	TEST("midioutpitch(2,0.2)", 2);
	TEST("midioutpot(3,1,3)", 3);
	TEST("midioutprog(4,7)", 4);
	TEST("encoder()+encoder(8)", 8);
	TEST("ay(1,2,3,4,5,6,7,8,9,10,11)", 2);
	TEST("sidfilter(0,1,2,3)", 1);
	TEST("sidvoice(5,6,7)", 6);
	TEST("sidenvelope(1,2,3,4,5)", 2);
	TEST("slewlimit(1,2)", 1);
	TEST("slewlimit(1,2,3)", 1);
	TEST("squaresaw(1.3,0.5,0.2)", squaresawd(1.3, 0.5, 0.2));
	TEST("akwf(1,2)", 2);
	TEST("bbd(1,2)", 2);
	TEST("bbd(3,4,5)", 4);
	TEST("hexwave(1,2,3,4,5)", 4);
//	TEST("out(", 0);
//	TEST("out(a1", 0);

//	TEST("d1:d13",0);

/*
	printf("double note_to_freq[128] = {");
	for (int i = 0; i < 128; i++)
		printf("%3.7f, ", pow(2, (i - 69) / 12.0) * 440);			
	printf("};\n");
*/

	printf("%d/%d tests ok (%d to go)\n", testcount - errors, testcount, errors);
	if (errors)
		return 0;
#endif

#if 0 // fuzz
	
	int cycle = 0;
	int mismatch = 0;
	int seed = GetTickCount();
	gRandom.init_genrand(seed);
	while (1)
	{
		cycle++;
		printf("\r--------- fuzz cycle %d --------- %d (%3.3f)", cycle, seed, (float)mismatch / cycle);
		mismatch += fuzz();
	}
#endif


#if 0
	unsigned int wfarray[4096];
	float bitmul = 0.8f; float bitstrength = 2.4f; float treshold = 0.64f;
//	float bitmul = 1.4f; float bitstrength = 1.9f; float treshold = 0.68f;
//	float bitmul = 0.8f; float bitstrength = 2.5f; float treshold = 0.64f;

//	createCombinedWF(TriSaw_8580,      0.8f, 2.4f, 0.64f);
//	createCombinedWF(PulseSaw_8580,    1.4f, 1.9f, 0.68f);
//	createCombinedWF(PulseTriSaw_8580, 0.8f, 2.5f, 0.64f);

	//void createCombinedWF(unsigned int* wfarray, float bitmul, float bitstrength, float treshold) {
	{
		int i, j, k;
		for (i = 0; i < 4096; i++) {
			wfarray[i] = 0;
			for (j = 0; j < 12; j++) {
				float bitlevel = 0;
				for (k = 0; k < 12; k++) {
					bitlevel += (float)(bitmul / pow(bitstrength, fabs(k - j))) * (((i >> k) & 1) - 0.5);
				}
				wfarray[i] += (float)((bitlevel >= treshold) ? pow(2, j) : 0);
			}
			wfarray[i] *= 12;
		}
	}
	for (int i = 0; i < 4096; i++)
	{
		printf("%d,", wfarray[i]);
		if (((i + 1) & 0x3f) == 0) printf("\n");
	}

#endif



	return 0;
}
