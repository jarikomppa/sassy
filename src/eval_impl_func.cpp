#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

#include <string>
#include <algorithm>
#include <map>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

const double note_to_freq[128] =
{ 8.1757989, 8.6619572, 9.1770240, 9.7227182, 10.3008612, 10.9133822, 11.5623257, 12.2498574, 12.9782718, 13.7500000, 14.5676175, 15.4338532,
16.3515978, 17.3239144, 18.3540480, 19.4454365, 20.6017223, 21.8267645, 23.1246514, 24.4997147, 25.9565436, 27.5000000, 29.1352351, 30.8677063,
32.7031957, 34.6478289, 36.7080960, 38.8908730, 41.2034446, 43.6535289, 46.2493028, 48.9994295, 51.9130872, 55.0000000, 58.2704702, 61.7354127,
65.4063913, 69.2956577, 73.4161920, 77.7817459, 82.4068892, 87.3070579, 92.4986057, 97.9988590, 103.8261744, 110.0000000, 116.5409404,
123.4708253, 130.8127827, 138.5913155, 146.8323840, 155.5634919, 164.8137785, 174.6141157, 184.9972114, 195.9977180, 207.6523488, 220.0000000,
233.0818808, 246.9416506, 261.6255653, 277.1826310, 293.6647679, 311.1269837, 329.6275569, 349.2282314, 369.9944227, 391.9954360, 415.3046976,
440.0000000, 466.1637615, 493.8833013, 523.2511306, 554.3652620, 587.3295358, 622.2539674, 659.2551138, 698.4564629, 739.9888454, 783.9908720,
830.6093952, 880.0000000, 932.3275230, 987.7666025, 1046.5022612, 1108.7305239, 1174.6590717, 1244.5079349, 1318.5102277, 1396.9129257,
1479.9776908, 1567.9817439, 1661.2187903, 1760.0000000, 1864.6550461, 1975.5332050, 2093.0045224, 2217.4610478, 2349.3181433, 2489.0158698,
2637.0204553, 2793.8258515, 2959.9553817, 3135.9634879, 3322.4375806, 3520.0000000, 3729.3100921, 3951.0664100, 4186.0090448, 4434.9220956,
4698.6362867, 4978.0317396, 5274.0409106, 5587.6517029, 5919.9107634, 6271.9269757, 6644.8751613, 7040.0000000, 7458.6201843, 7902.1328201,
8372.0180896, 8869.8441913, 9397.2725734, 9956.0634791, 10548.0818212, 11175.3034059, 11839.8215268, 12543.8539514 };


std::map<std::string, int> gSymbolMap;
std::map<int, std::string> gFunctionNameMap;
std::map<std::string, int> gFunctionMap;

void add_function_permutations(const char* fn, const char* args, int funcno)
{
	char digits[4] = "CLV";
	char work[16];
	strcpy(work, args);
	char temp[64];
	sprintf(temp, "%s@%s", fn, args);
	add_function(temp, funcno);
	int itercount = 1;
	int ccount = 0;
	char* s = work;
	while (*s)
	{
		if (*s == 'C')
		{
			itercount *= 3;
			ccount++;
		}
		s++;
	}
	if (itercount > 1)
	{
		s = work;
		while (*s != 'C') s++;
		for (int i = 1; i < itercount; i++)
		{
			char* p = s;
			int v = i;
			for (int j = 0; j < ccount; j++)
			{
				*p = digits[v % 3];
				p++;
				v /= 3;
			}
			sprintf(temp, "%s@%s", fn, work);
			add_function(temp, funcno);
		}
	}
}

void init_symbols()
{
	gFunctionMap.clear();
	gSymbolMap.clear();
	gFunctionNameMap.clear();
	for (int j = 0; j < MAXROWS; j++)
	{
		for (int i = 0; i < 26; i++)
		{
			char temp[32];
			sprintf_s(temp, 32, "%c%d", 'A' + i, 1 + j);
			add_symbol(temp, j * 27 + i + 1); // variable
			sprintf_s(temp, 32, "$%c%d", 'A' + i, 1 + j);
			add_symbol(temp, j * 27 + i + 1); // variable
			sprintf_s(temp, 32, "%c$%d", 'A' + i, 1 + j);
			add_symbol(temp, j * 27 + i + 1); // variable
			sprintf_s(temp, 32, "$%c$%d", 'A' + i, 1 + j);
			add_symbol(temp, j * 27 + i + 1); // variable
		}
	}
	for (int i = 1; i < FUNC_LAST; i++)
	{
		add_symbol(gFunc[i].mName, -i); // function
		add_function_permutations(gFunc[i].mName, gFunc[i].mArgs, i);
	}
}

void add_function(const char* sym, int symtype)
{
	std::string s = sym;
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	gFunctionMap[s] = symtype;
}

void add_symbol(const char* sym, int symtype)
{
	std::string s = sym;
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	gSymbolMap[s] = symtype;
	if (symtype < 0)
		gFunctionNameMap[-symtype] = s;
}

int get_function(int funcsym, const char* sig)
{
	std::string s = gFunctionNameMap[funcsym] + "@" + sig;
	if (gFunctionMap.find(s) == gFunctionMap.end())
		return 0;
	return gFunctionMap[s];
}

int match_partial(const char* partial, const char* whole)
{
	while (*partial && *whole && toupper(*partial) == *whole)
	{
		partial++;
		whole++;
	}
	return !*partial;
}

const char* find_tab(const char* partial)
{
	for (auto &x : gFunctionNameMap)
	{
		if (match_partial(partial, x.second.c_str()))
		{
			return x.second.c_str() + strlen(partial);
		}
	}
	return 0;
}

int is_symbol(const char* sym)
{
	std::string s = sym;
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	if (gSymbolMap.find(s) == gSymbolMap.end())
		return 0;
	return gSymbolMap[s];
}

void remove_symbol(const char* sym)
{
	std::string s = sym;
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
	gSymbolMap.erase(s);
}

namespace EvalFunc
{

	double sin1(double v)
	{
		v -= (int)v;
		return sin(v * M_PI * 2.0) * 0.5;
	}

	double triangle(double v)
	{
		v -= (int)v;
		return (v > 0.5 ? (1.0 - (v - 0.5) * 2) : v * 2.0) - 0.5;
	}

	double square(double v)
	{
		v -= (int)v;
		return v > 0.5 ? 0.5 : -0.5;
	}

	double squareq(double v)
	{
		v -= (int)v;
		double f = 0;
		for (int i = 1; i < 22; i += 2)
		{
			f += (4.0 / (M_PI * i)) * sin(2.0 * M_PI * i * v);
		}
		return f * 0.5;
	}

	double squaref(double v, double o)
	{
		double f = 0;
		if (o < 0) o = 0;
		int n = (int)o + 1;
		if (n > 50) n = 50;
		v -= (int)v;
		for (int i = 1; i < n * 2; i += 2)
		{
			f += (4.0 / (M_PI * i)) * sin(2.0 * M_PI * i * v);
		}
		return f * 0.5;
	}

	double saw(double v)
	{
		if (v < 0) v = -v;
		v -= (int)v;
		return v - 0.5;
	}

	double sawq(double v)
	{
		double f = 0;
		if (v < 0) v = -v;
		v -= (int)v;
		for (int i = 1; i < 15; i++)
		{
			if (i & 1)
				f += (1.0 / (M_PI * i)) * sin(v * 2.0 * M_PI * i);
			else
				f -= (1.0 / (M_PI * i)) * sin(v * 2.0 * M_PI * i);
		}
		return f;
	}

	double sawf(double v, double o)
	{
		double f = 0;
		if (v < 0) v = -v;
		if (o < 0) o = 0;
		int n = (int)o + 1;
		if (n > 50) n = 50;
		v -= (int)v;
		for (int i = 1; i < n; i++)
		{
			if (i & 1)
				f += (1.0 / (M_PI * i)) * sin(v * 2.0 * M_PI * i);
			else
				f -= (1.0 / (M_PI * i)) * sin(v * 2.0 * M_PI * i);
		}
		return f;
	}

	double pulse(double v, double d)
	{
		if (v < 0) v = -v;
		v -= (int)v;
		return v > d ? 0.5 : -0.5;
	}

	double supersaw(double v, double s, double d)
	{
		double f = 0;
		double scale = 1;
		double detune = 1;
		v -= (int)v;
		for (int i = 0; i < 3; i++)
		{
			f += saw(v * detune) * scale;
			detune *= 2 - d;
			scale *= s;
		}
		return f;
	}

	double supersquare(double v, double s, double d)
	{
		double f = 0;
		double scale = 1;
		double detune = 1;
		v -= (int)v;
		for (int i = 0; i < 3; i++)
		{
			f += square(v * detune) * scale;
			detune *= 2 - d;
			scale *= s;
		}
		return f;
	}

	double supersin(double v, double s, double d)
	{
		double f = 0;
		double scale = 1;
		double detune = 1;
		v -= (int)v;
		for (int i = 0; i < 3; i++)
		{
			f += sin1(v * detune) * scale;
			detune *= 2 - d;
			scale *= s;
		}
		return f;
	}

	static const double squaresaw_vecs[8 * 6] =
	{
		0.0,  0.5, // square
		0.5,  0.5,
		0.5, -0.5,
		1.0, -0.5,

		0.0, -0.5, // saw target for square
		0.5,  0,
		0.5,  0,
		1.0,  0.5,

		// saw-tri-invsaw

		0.0, -0.5, // saw
		1.0, 0.5,
		1.0, -0.5,
		1.0, -0.5,

		0.0, -0.5, // triangle
		0.5, 0.5,
		1.0, -0.5,
		1.0, -0.5,

		0.0, -0.5, // inverse saw
		0.0, 0.5,
		1.0, -0.5,
		1.0, -0.5,

		0.0, 0.5, // inverse saw for square
		0.0, 0.5,
		1.0, -0.5,
		1.0, -0.5,
	};

	double squaresaw(double v, double f)
	{
		if (f != f) f = 0;
		if (v < 0) v = -v;
		if (f < 0) f = -f;
		v -= (int)v;
		int mode1 = (int)f % 4;
		int mode2 = (mode1 + 1) % 4;
		if (mode1 > 0)
		{
			mode1 = mode1 + 1;
			mode2 = mode1 + 1;
		}
		if (mode1 == 4)
		{
			mode1 = 5;
			mode2 = 0;
		}

		f = f - (int)f;

		double p[8];
		for (int i = 0; i < 8; i++)
			p[i] = squaresaw_vecs[mode1 * 8 + i] + (squaresaw_vecs[mode2 * 8 + i] - squaresaw_vecs[mode1 * 8 + i]) * f;

		int q = 0;
		while (p[(q + 1) * 2] < v) q++;

		v = (v - p[q * 2]) / (p[(q + 1) * 2] - p[q * 2]);

		return p[q * 2 + 1] + (p[q * 2 + 3] - p[q * 2 + 1]) * v;

	}

	double squaresawd(double v, double f, double d)
	{
		if (f != f) f = 0;
		if (v < 0) v = -v;
		if (f < 0) f = -f;
		d -= (int)d;
		v -= (int)v;
		int mode1 = (int)f % 4;
		int mode2 = (mode1 + 1) % 4;
		if (mode1 > 0)
		{
			mode1 = mode1 + 1;
			mode2 = mode1 + 1;
		}
		if (mode1 == 4)
		{
			mode1 = 5;
			mode2 = 0;
		}

		f = f - (int)f;

		double p[8];
		for (int i = 0; i < 8; i++)
		{
			double v1 = squaresaw_vecs[mode1 * 8 + i];
			double v2 = squaresaw_vecs[mode2 * 8 + i];
			if ((i & 1) == 0 && v1 == 0.5) v1 = d;
			if ((i & 1) == 0 && v2 == 0.5) v2 = d;
			p[i] = v1 + (v2 - v1) * f;
		}

		int q = 0;
		while (p[(q + 1) * 2] < v) q++;

		v = (v - p[q * 2]) / (p[(q + 1) * 2] - p[q * 2]);

		return p[q * 2 + 1] + (p[q * 2 + 3] - p[q * 2 + 1]) * v;

	}

	double opl1(double v)
	{
		v -= (int)v;
		if (v > 0.5) return 0;
		return sin(v * M_PI * 2.0) * 0.5;
	}

	double opl2(double v)
	{
		v -= (int)v;
		return abs(sin(v * M_PI * 2.0) * 0.5);
	}

	double opl3(double v)
	{
		v -= (int)v;
		if (v > 0.25 && v < 0.5 ||
			v > 0.75) return 0;
		return abs(sin(v * M_PI * 2.0) * 0.5);
	}

	double b_and(double a, double b)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vb = (unsigned int)b;
		return (double)(va & vb);
	}

	double b_or(double a, double b)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vb = (unsigned int)b;
		return (double)(va | vb);
	}

	double b_not(double a)
	{
		unsigned int va = (unsigned int)a;
		return (double)(unsigned int)(~va);
	}

	double b_xor(double a, double b)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vb = (unsigned int)b;
		return (double)(va ^ vb);
	}

	double b_nand(double a, double b)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vb = (unsigned int)b;
		return (double)(unsigned int)(~(va & vb));
	}

	double b_nor(double a, double b)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vb = (unsigned int)b;
		return (double)(unsigned int)(~(va | vb));
	}

	double b_shleft(double a, double amt)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vamt = (unsigned int)amt;
		return (double)(va << vamt);
	}

	double b_shright(double a, double amt)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vamt = (unsigned int)amt;
		return (double)(va >> vamt);
	}

	double b_rotleft(double a, double amt, double bits)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vamt = (unsigned int)amt;
		unsigned int vbits = (unsigned int)bits;
		if (vbits)
			vamt %= vbits;
		return (double)((va << vamt) | (va >> (vbits - vamt)));
	}

	double b_rotright(double a, double amt, double bits)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vamt = (unsigned int)amt;
		unsigned int vbits = (unsigned int)bits;

		if (vbits)
			vamt %= vbits;
		return (double)((va >> vamt) | (va << (vbits - vamt)));
	}

	double b_test(double a, double bit)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vbit = (unsigned int)bit;
		if (va & (1 << vbit))
			return 1;
		return 0;
	}

	double b_set(double a, double bit)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vbit = (unsigned int)bit;
		return (double)(va | (1 << vbit));
	}

	double b_clear(double a, double bit)
	{
		unsigned int va = (unsigned int)a;
		unsigned int vbit = (unsigned int)bit;
		return (double)(va & ~(1 << vbit));
	}

	double rgb(double r, double g, double b)
	{
		return (double)
			(((int)(r * 0xff) << 16) |
				((int)(g * 0xff) << 8) |
				((int)(b * 0xff) << 0));
	}

	double bitcrunch(double v, double bits)
	{
		double scaler = pow(2, bits);
		return (double((int)(v * scaler))) / scaler;
	}

	int gScaleData[] = {
	#include "data/scales.h"
	};
	int gScaleDataSize = sizeof(gScaleData) / sizeof(int);

	double scale(double note, double scale)
	{
		int n = (int)note;
		int s = (int)scale;
		if (n < 0 || n > 127) return 0;
		if (s < 0 || s >(gScaleDataSize / 128)) return 0;
		return gScaleData[s * 128 + n];
	}

	double distort(double v)
	{
		// sign(v) * ( 1 - e ^ -|v|)
		return ((0.0 < v) - (v < 0.0)) * (1.0 - exp(-abs(v)));
	}

	double catmullrom(double t, double p0, double p1, double p2, double p3)
	{
		return 0.5 * (
			(2 * p1) +
			(-p0 + p2) * t +
			(2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
			(-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t
			);
	}
};