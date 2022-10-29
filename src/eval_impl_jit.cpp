// eval_impl_jit.cpp

#if defined(__APPLE__)
// Avoid a name-conflict between our 'label' function and the
// one defined in this header file.
#define label IGNORE_LABEL
#include <sys/ucred.h>
#undef label
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"
#include "xbyak/xbyak.h"

using namespace EvalFunc;

namespace jit_kludge
{
	double log(double v)
	{
		return std::log((float)v); // no "double log(double)" exists
	}

	double log10(double v)
	{
		return std::log10((float)v); // no "double log10(double)" exists
	}

	double tan(double v)
	{
		return std::tan((float)v); // no "double tan(double)" exists
	}

	double notetofreq(double v)
	{
		if ((int)v >= 0 && (int)v < 128)
			return note_to_freq[(int)v];
		return 0;
	}

	double freqtonote(double v)
	{
		return 12 * log(32 * POW_2_3_4TH * (v / 440)) / log(2);
	}

	double even(double v)
	{
		return ((int)v & 1) ? 0.0 : 1.0;
	}

	double odd(double v)
	{
		return ((int)v & 1) ? 1.0 : 0.0;
	}

	double sign(double v)
	{
		return (v < 0.0) ? -1.0 : 1.0;
	}

	double smoothstep(double v)
	{
		return v * v * (3 - 2 * v);
	}

	double smootherstep(double v)
	{
		return v * v * v * (v * (v * 6 - 15) + 10);
	}

	double notetofreqslow(double v)
	{
		return pow(2, (v - 69) / 12.0) * 440;
	}

	double quantize(double v)
	{
		double t =  12 * log(32 * POW_2_3_4TH * (v) / 440) / log(2);
		int i = (int)t;
		if (i >= 0 && i < 128)
			return note_to_freq[i];
		return v;
	}

	double minfunc(double a, double b)
	{
		return (b < a) ? b : a;
	}

	double maxfunc(double a, double b)
	{
		return (b > a) ? b : a;
	}

	double clamp(double a, double b, double c)
	{
		return (a < b) ? b : (a > c) ? c : a;
	}

	double mix(double a, double b, double c)
	{
		return b + a * (c - b);
	}

	double map(double a, double b, double c, double d, double e)
	{
		return ((a - b) / (c - b)) * (e - d) + d;
	} 

	double pinknoise(int cell, int memofs)
	{
		return paranoise(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, cell, memofs);
	}

	double brownnoise(int cell, int memofs)
	{
		return paranoise(10, 9, 8, 7, 6, 5, 4, 3, 2, 1, cell, memofs);
	}

	double bluenoise(int cell, int memofs)
	{
		return paranoise(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, cell, memofs);
	}

}

void store_xmm0(Xbyak::CodeGenerator& code, bool& first)
{
	if (first)
	{
		first = false;
		return;
	}
	using namespace Xbyak::util;
	code.sub(rsp, 16); // push xmm0 to stack
	code.movq(qword[rsp + 32], xmm0);
}

void jit_init(Xbyak::CodeGenerator& code)
{
	using namespace Xbyak::util;

	code.resetSize();
	code.sub(rsp, 16 * 10);
	code.movdqu(xword[rsp + 16 * 0], xmm6);
	code.movdqu(xword[rsp + 16 * 1], xmm7);
	code.movdqu(xword[rsp + 16 * 2], xmm8);
	code.movdqu(xword[rsp + 16 * 3], xmm9);
	code.movdqu(xword[rsp + 16 * 4], xmm10);
	code.movdqu(xword[rsp + 16 * 5], xmm11);
	code.movdqu(xword[rsp + 16 * 6], xmm12);
	code.movdqu(xword[rsp + 16 * 7], xmm13);
	code.movdqu(xword[rsp + 16 * 8], xmm14);
	code.movdqu(xword[rsp + 16 * 9], xmm15);
	code.push(rbx);
	code.push(rbp);
	code.push(rdi);
	code.push(r12);
	code.push(r13);
	code.push(r14);
	code.push(r15);
	code.sub(rsp, 32); // reserve stack for func calls

	code.mov(r14, rcx); // read var ptr
	code.mov(r15, rdx); // write var ptr
}

int jit_addcode(Op* bc, int cell, Xbyak::CodeGenerator& code, char *stringstore)
{
	using namespace Xbyak::util;
	union double_64int
	{
		double f;
		unsigned long long i64;
	};

	double_64int one, epsilon;
	one.f = 1;
	epsilon.f = EPSILON;

	// Assumption:
// Top of stack == xmm0.
// rsp is kept at -32 bytes offset so we don't need to muck with it doing calls.

	int memofs = 0;
	int stack = 0;
	bool first = 1;

	if (bc->mOpcode == 0)
		return 0;
	while (bc->mOpcode)
	{
		switch (bc->mOpcode)
		{
		case 'C': // numeric constant
		{
			store_xmm0(code, first);
			if (bc->mArg.f64)
			{
				code.mov(rax, bc->mArg.f64); // move constant to xmm0
				code.movq(xmm0, rax);
			}
			else
			{
				code.xorpd(xmm0, xmm0);
			}
			stack++;
			bc++;
		}
		break;
		case 'A': // area
		{
			double_64int t;
			store_xmm0(code, first);
			t.f = bc->mArg.i.i;
			code.mov(rax, t.i64); // move constant to xmm0
			code.movq(xmm0, rax);

			code.sub(rsp, 16); // push xmm0 to stack
			code.movq(qword[rsp + 32], xmm0);
			t.f = bc->mArg.i.j;
			code.mov(rax, t.i64); // move constant to xmm0
			code.movq(xmm0, rax);
			stack += 2;
			bc++;
		}
		break;
		case '+':
		{
			stack--;
			// Pop two values off stack
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.addsd(xmm0, xmm1);
			bc++;
		}
		break;
		case '-':
		{
			stack--;
			// Pop two values off stack
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.subsd(xmm1, xmm0); // arguments are swapped, but don't matter
			code.movsd(xmm0, xmm1);
			bc++;
		}
		break;
		case '*':
		{
			stack--;
			// Pop two values off stack
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.mulsd(xmm0, xmm1); // arguments are swapped, but don't matter
			bc++;
		}
		break;
		case '/':
		{
			stack--;
			// Pop two values off stack
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.divsd(xmm1, xmm0); // order matters!
			code.movsd(xmm0, xmm1);
			bc++;
		}
		break;
		case '>':
		{
			stack--;
			// Compare is swapped because parameters are in wrong regs
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.cmpsd(xmm0, xmm1, 1);
			code.mov(rax, one.i64);
			code.movq(xmm1, rax);
			code.andpd(xmm0, xmm1);
			bc++;
		}
		break;
		case '<':
		{
			stack--;
			// Compare is swapped because parameters are in wrong regs
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.cmpsd(xmm0, xmm1, 5);
			code.mov(rax, one.i64);
			code.movq(xmm1, rax);
			code.andpd(xmm0, xmm1);
			bc++;
		}
		break;
		case '=': // TODO: no threshold check
		{
			stack--;
			// Pop two values off stack
			code.movq(xmm1, qword[rsp + 32]);
			code.add(rsp, 16);
			code.cmpsd(xmm0, xmm1, 0); // leads to empty or full mask (Arguments are swapped but don't matter)
			code.mov(rax, one.i64);
			code.movq(xmm1, rax);
			code.andpd(xmm0, xmm1); // anding pattern of 1.0 to either 0.0 or 1.0
			bc++;
		}
		break;
		case 'V': // variable
		{
			store_xmm0(code, first);
			code.movq(xmm0, qword[r14 + (bc->mArg.i.i - 1) * 8]);
			stack++;
			bc++;
		}
		break;
		case 'T': // text
		{
			store_xmm0(code, first);
			long long v = ((intptr_t)stringstore + bc->mArg.i.i);
			code.mov(rax, v);
			code.movq(xmm0, rax);
			stack++;
			bc++;
		}
		break;

		case 'F': // function call
		{
#define CALLFUNC1(x) \
			code.mov(rax, (size_t)static_cast<double(*)(double)>(x)); \
			code.call(rax); 

			int func = bc->mArg.i.i;
			bc++;
			switch (func)
			{
			case FUNC_SIN:
				CALLFUNC1(sin);
				break;
			case FUNC_OUT:
				CALLFUNC1(output);
				break;
			case FUNC_TRUNC:
				CALLFUNC1(trunc);
				break;
			case FUNC_FRACT: // this is overly complicated, should calc trunc/fract without func call
				code.movsd(xmm1, xmm0);
				CALLFUNC1(trunc);
				code.movsd(xmm2, xmm0);
				code.movsd(xmm0, xmm1);
				code.subsd(xmm0, xmm2);
				//stack[sp - 1] = stack[sp - 1] - trunc(stack[sp - 1]);
				break;

			case FUNC_SIN1:
				CALLFUNC1(sin1);
				break;
			case FUNC_TRIANGLE:
				CALLFUNC1(triangle);
				break;
			case FUNC_SQUARE:
				CALLFUNC1(square);
				break;
			case FUNC_SQUAREQ:
				CALLFUNC1(squareq);
				break;
			case FUNC_SAW:
				CALLFUNC1(saw);
				break;
			case FUNC_SAWQ:
				CALLFUNC1(sawq);
				break;
			case FUNC_SQRT:
				CALLFUNC1(sqrt);
				break;
			case FUNC_LOG:
				CALLFUNC1(jit_kludge::log);
				break;
			case FUNC_COS:
				CALLFUNC1(cos);
				break;
			case FUNC_TAN:
				CALLFUNC1(jit_kludge::tan);
				break;
			case FUNC_ACOS:
				CALLFUNC1(acos);
				break;
			case FUNC_ASIN:
				CALLFUNC1(asin);
				break;
			case FUNC_ATAN:
				CALLFUNC1(atan);
				break;
			case FUNC_COSH:
				CALLFUNC1(cosh);
				break;
			case FUNC_SINH:
				CALLFUNC1(sinh);
				break;
			case FUNC_TANH:
				CALLFUNC1(tanh);
				break;
			case FUNC_EXP:
				CALLFUNC1(exp);
				break;
			case FUNC_LOG10:
				CALLFUNC1(jit_kludge::log10);
				break;
			case FUNC_FLOOR:
				CALLFUNC1(floor);
				break;
			case FUNC_ABS:
				CALLFUNC1(abs);
				break;
			case FUNC_OPL1:
				CALLFUNC1(opl1);
				break;
			case FUNC_OPL2:
				CALLFUNC1(opl2);
				break;
			case FUNC_OPL3:
				CALLFUNC1(opl3);
				break;
			case FUNC_B_NOT:
				CALLFUNC1(b_not);
				break;
			case FUNC_NOTETOFREQ:
				CALLFUNC1(jit_kludge::notetofreq);
				break;
			case FUNC_FREQTONOTE:
				CALLFUNC1(jit_kludge::freqtonote);
				break;
			case FUNC_EVEN:
				CALLFUNC1(jit_kludge::even);
				break;
			case FUNC_ODD:
				CALLFUNC1(jit_kludge::odd);
				break;
			case FUNC_SIGN:
				CALLFUNC1(jit_kludge::sign);
				break;
			case FUNC_SMOOTHSTEP:
				CALLFUNC1(jit_kludge::smoothstep);
				break;
			case FUNC_SMOOTHERSTEP:
				CALLFUNC1(jit_kludge::smootherstep);
				break;
			case FUNC_NOTETOFREQSLOW:
				CALLFUNC1(jit_kludge::notetofreqslow);
				break;
			case FUNC_QUANTIZE:
				CALLFUNC1(jit_kludge::quantize);
				break;
			case FUNC_MIDIOUTPITCH:
				CALLFUNC1(midioutpitch);
				break;
			case FUNC_MIDIOUTPROG:
				CALLFUNC1(midioutprog);
				break;
			case FUNC_IN2:
				CALLFUNC1(in2);
				break;
			case FUNC_MIDIVALV:
				CALLFUNC1(midivalv);
				break;
			case FUNC_MIDINOTEV:
				CALLFUNC1(midinotev);
				break;
			case FUNC_MIDIVELV:
				CALLFUNC1(midivelv);
				break;
			case FUNC_MIDIONV:
				CALLFUNC1(midionv);
				break;
			case FUNC_MIDICHANNEL:
				CALLFUNC1(midichannel);
				break;
			case FUNC_DISTORT:
				CALLFUNC1(distort);
				break;
			case FUNC_LEN:
				CALLFUNC1(len);
				break;
			case FUNC_CHANNELS:
				CALLFUNC1(channels);
				break;

#define CALLFUNC2(x) \
			stack--; \
			code.movsd(xmm1, xmm0); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(rax, (size_t)static_cast<double(*)(double, double)>(x)); \
			code.call(rax);

			case FUNC_OUT2:
				CALLFUNC2(output2);
				break;
			case FUNC_PULSE:
				CALLFUNC2(pulse);
				break;
			case FUNC_POW:
				CALLFUNC2(pow);
				break;
			case FUNC_ATAN2:
				CALLFUNC2(atan2);
				break;
			case FUNC_SQUAREF:
				CALLFUNC2(squaref);
				break;
			case FUNC_SAWF:
				CALLFUNC2(sawf);
				break;
			case FUNC_SQUARESAW:
				CALLFUNC2(squaresaw);
				break;
			case FUNC_B_AND:
				CALLFUNC2(b_and);
				break;
			case FUNC_B_OR:
				CALLFUNC2(b_or);
				break;
			case FUNC_B_XOR:
				CALLFUNC2(b_xor);
				break;
			case FUNC_B_NAND:
				CALLFUNC2(b_nand);
				break;
			case FUNC_B_NOR:
				CALLFUNC2(b_nor);
				break;
			case FUNC_B_SHLEFT:
				CALLFUNC2(b_shleft);
				break;
			case FUNC_B_SHRIGHT:
				CALLFUNC2(b_shright);
				break;
			case FUNC_B_TEST:
				CALLFUNC2(b_test);
				break;
			case FUNC_B_SET:
				CALLFUNC2(b_set);
				break;
			case FUNC_B_CLEAR:
				CALLFUNC2(b_clear);
				break;
			case FUNC_LOOKUP:
				CALLFUNC2(lookup);
				break;
			case FUNC_AREAMIN:
				CALLFUNC2(tablemin);
				break;
			case FUNC_AREAMAX:
				CALLFUNC2(tablemax);
				break;
			case FUNC_AVERAGE:
				CALLFUNC2(tableaverage);
				break;
			case FUNC_SUM:
				CALLFUNC2(tablesum);
				break;
			case FUNC_PRODUCT:
				CALLFUNC2(tableproduct);
				break;
			case FUNC_COUNT:
				CALLFUNC2(tablecount);
				break;
			case FUNC_MOD:
				CALLFUNC2(fmod);
				break;
			case FUNC_MIN:
				CALLFUNC2(jit_kludge::minfunc);
				break;
			case FUNC_MAX:
				CALLFUNC2(jit_kludge::maxfunc);
				break;
			case FUNC_MIDIOUT:
				CALLFUNC2(midiout);
				break;
			case FUNC_MIDIOUTPOT:
				CALLFUNC2(midioutpot);
				break;
			case FUNC_PROBEC:
				CALLFUNC2(probe);
				break;
			case FUNC_BITCRUNCH:
				CALLFUNC2(bitcrunch);
				break;
			case FUNC_MIDIVALC:
				CALLFUNC2(midivalc);
				break;
			case FUNC_MIDINOTEC:
				CALLFUNC2(midinotec);
				break;
			case FUNC_MIDIVELC:
				CALLFUNC2(midivelc);
				break;
			case FUNC_MICIONC:
				CALLFUNC2(midionc);
				break;
			case FUNC_SCALE:
				CALLFUNC2(scale);
				break;
			case FUNC_SAMPLE:
				CALLFUNC2(sample);
				break;
			case FUNC_SAMPLEFAST:
				CALLFUNC2(samplefast);
				break;
			case FUNC_MIDIOUTPITCHC:
				CALLFUNC2(midioutpitchc);
				break;
			case FUNC_MIDIOUTPROGC:
				CALLFUNC2(midioutprogc);
				break;
			case FUNC_AKWF:
				CALLFUNC2(akwf);
				break;


#define PUSH_CONST(x) \
			{\
				store_xmm0(code, first); \
				double_64int t; t.f = x; \
				stack++; \
				code.mov(rax, t.i64); /* move constant to xmm0 */ \
				code.movq(xmm0, rax); \
			}

			case FUNC_PI:
				PUSH_CONST(M_PI);
				break;
			case FUNC_NAN:
				PUSH_CONST(nan("0"));
				break;
			case FUNC_TRUE:
				PUSH_CONST(1.0);
				break;
			case FUNC_FALSE:
				PUSH_CONST(0.0);
				break;
			case FUNC_DT:
				PUSH_CONST(dt());
				break;

#define CALLFUNC0(x) \
			stack++; \
			store_xmm0(code, first); \
			code.mov(rax, (size_t)static_cast<double(*)()>(x)); \
			code.call(rax);

			case FUNC_TIME:
				CALLFUNC0(gettime);
				break;
			case FUNC_MIDIVAL:
				CALLFUNC0(midival);
				break;
			case FUNC_MIDINOTE:
				CALLFUNC0(midinote);
				break;
			case FUNC_MIDIVEL:
				CALLFUNC0(midivel);
				break;
			case FUNC_MIDION:
				CALLFUNC0(midion);
				break;
			case FUNC_MIDIPITCH:
				CALLFUNC0(midipitch);
				break;
			case FUNC_MIDIPROG:
				CALLFUNC0(midiprog);
				break;
			case FUNC_NOISE:
				CALLFUNC0(noise);
				break;
			case FUNC_IN:
				CALLFUNC0(in);
				break;
			case FUNC_CPU:
				CALLFUNC0(cpu);
				break;

#define CALLFUNC1M(x) \
			code.mov(rdx, cell); /* set up memory params */ \
			code.mov(r8, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(double,int,int)>(x)); \
			code.call(rax);

			case FUNC_GRAPH:
				CALLFUNC1M(graph);
				break;
			case FUNC_FFT:
				CALLFUNC1M(fft);
				break;
			case FUNC_TIMEGATE:
				CALLFUNC1M(timegate);
				break;
			case FUNC_BAR:
				CALLFUNC1M(bar);
				break;
			case FUNC_PIE:
				CALLFUNC1M(pie);
				break;
			case FUNC_SLIDERV:
				CALLFUNC1M(slider);
				break;
			case FUNC_TOGGLEV:
				CALLFUNC1M(toggle);
				break;
			case FUNC_STEP:
				CALLFUNC1M(step);
				break;
			case FUNC_DC:
				CALLFUNC1M(dc);
				break;
			case FUNC_PLOTXY:
				CALLFUNC1M(plotxy);
				break;
			case FUNC_BUFFER:
				CALLFUNC1M(buffer);
				break;
			case FUNC_ENCODERV:
				CALLFUNC1M(encoder);
				break;

#define CALLFUNC1MT(x) \
			code.movq(rcx, xmm0); \
			code.mov(rdx, cell); /* set up memory params */ \
			code.mov(r8, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(char*,int,int)>(x)); \
			code.call(rax);

			case FUNC_LOADWAV:
				CALLFUNC1MT(loadwav);
				break;
			case FUNC_KLATT:
				CALLFUNC1MT(klatt);
				break;
			case FUNC_LABEL:
				CALLFUNC1MT(label);
				break;

#define CALLFUNC2M(x) \
			stack--; \
			code.movsd(xmm1, xmm0); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(r8, cell); /* set up memory params */ \
			code.mov(r9, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(double,double,int,int)>(x)); \
			code.call(rax); 

			case FUNC_DELAY:
				CALLFUNC2M(delay);
				break;
			case FUNC_HOLD:
				CALLFUNC2M(hold);
				break;
			case FUNC_LATCH:
				CALLFUNC2M(latch);
				break;
			case FUNC_MIDIPOTV:
				CALLFUNC2M(midipot);
				break;
			case FUNC_TRIGGERM:
				CALLFUNC2M(trigger);
				break;
			case FUNC_RUBBERBAND:
				CALLFUNC2M(rubberband);
				break;
			case FUNC_SLIDERPOTV:
				CALLFUNC2M(sliderpot);
				break;
			case FUNC_DRUNKARDSWALK:
				CALLFUNC2M(drunkardswalk);
				break;
			case FUNC_TOGGLEPOTV:
				CALLFUNC2M(togglepot);
				break;
			case FUNC_STEPG:
				CALLFUNC2M(stepg);
				break;
			case FUNC_PLOTXY2:
				CALLFUNC2M(plotxy2);
				break;
			case FUNC_BARA:
				CALLFUNC2M(bara);
				break;
			case FUNC_PIEA:
				CALLFUNC2M(piea);
				break;
			case FUNC_PLAY:
				CALLFUNC2M(play);
				break;
			case FUNC_SLEWLIMIT:
				CALLFUNC2M(slewlimit);
				break;
			case FUNC_BBD:
				CALLFUNC2M(bbd);
				break;

#define CALLFUNC2MT(x) \
			stack--; \
			code.movsd(xmm1, xmm0); \
			code.mov(rcx, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(r8, cell); /* set up memory params */ \
			code.mov(r9, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(char*,double,int,int)>(x)); \
			code.call(rax); 

			case FUNC_LOADWAVC:
				CALLFUNC2MT(loadwavc);
				break;


#define CALLFUNC3M(x) \
			stack -= 2; \
			code.movsd(xmm2, xmm0); \
			code.movq(xmm1, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(r9, cell); /* set up memory params */ \
			code.sub(rsp, 8); /* 16 byte align*/ \
			code.sub(rsp, 8); \
			code.mov(dword[rsp + 32], memofs);  /* push memofs to stack */ \
			code.mov(rax, (size_t)static_cast<double(*)(double, double, double, int, int)>(x)); \
			code.call(rax); \
			code.add(rsp, 16); /* memofs + align off stack */

			case FUNC_HOLDL:
				CALLFUNC3M(holdl);
				break;
			case FUNC_PLAYLOOP:
				CALLFUNC3M(playloop);
				break;
			case FUNC_SIDVOICE:
				CALLFUNC3M(sidvoice);
				break;
			case FUNC_SLEWLIMITUD:
				CALLFUNC3M(slewlimitud);
				break;
			case FUNC_BBDEFF:
				CALLFUNC3M(bbdeff);
				break;

#define CALLFUNC3TM(x) \
			stack -= 2; \
			code.movq(xmm2, xmm0); \
			code.movq(xmm1, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(rcx, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(r9, cell); /* set up memory params */ \
			code.sub(rsp, 8); /* 16 byte align*/ \
			code.sub(rsp, 8); \
			code.mov(dword[rsp + 32], memofs);  /* push memofs to stack */ \
			code.mov(rax, (size_t)static_cast<double(*)(char*, double, double, int, int)>(x)); \
			code.call(rax); \
			code.add(rsp, 16); /* memofs + align off stack */

			case FUNC_IMG:
				CALLFUNC3TM(img);
				break;

#define CALLFUNC3(x) \
			stack -= 2; \
			code.movsd(xmm2, xmm0); \
			code.movq(xmm1, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(rax, (size_t)static_cast<double(*)(double, double, double)>(x)); \
			code.call(rax);

			case FUNC_B_ROTLEFTV:
				CALLFUNC3(b_rotleft);
				break;
			case FUNC_B_ROTRIGHTV:
				CALLFUNC3(b_rotright);
				break;
			case FUNC_HILIGHTV:
				CALLFUNC3(hilight);
				break;
			case FUNC_RGB:
				CALLFUNC3(rgb);
				break;
			case FUNC_REPLACE:
				CALLFUNC3(replace);
				break;
			case FUNC_SUPERSAW:
				CALLFUNC3(supersaw);
				break;
			case FUNC_SUPERSQUARE:
				CALLFUNC3(supersquare);
				break;
			case FUNC_SUPERSIN:
				CALLFUNC3(supersin);
				break;
			case FUNC_SELECT:
				CALLFUNC3(tableselect);
				break;
			case FUNC_SELECTV:
				CALLFUNC3(tableselectv);
				break;
			case FUNC_FIND:
				CALLFUNC3(tablefind);
				break;
			case FUNC_FINDV:
				CALLFUNC3(tablefindv);
				break;
			case FUNC_CLAMP:
				CALLFUNC3(jit_kludge::clamp);
				break;
			case FUNC_MIX:
				CALLFUNC3(jit_kludge::mix);
				break;
			case FUNC_MIDIOUTC:
				CALLFUNC3(midioutc);
				break;
			case FUNC_MIDIOUTPOTC:
				CALLFUNC3(midioutpotc);
				break;
			case FUNC_SQUARESAWD:
				CALLFUNC3(squaresawd);
				break;

#define CALLFUNC4(x) \
			stack -= 3; \
			code.movsd(xmm3, xmm0); \
			code.movq(xmm2, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm1, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double)>(x)); \
			code.call(rax);

			case FUNC_MIDIOUTRAW:
				CALLFUNC4(midioutraw);
				break;
			case FUNC_WRITE:
				CALLFUNC4(write);
				break;

#define CALLFUNC4M(x) \
			stack -= 3; \
			code.movsd(xmm3, xmm0); \
			code.movq(xmm2, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm1, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.sub(rsp, 8); \
			code.mov(dword[rsp + 32], memofs); \
			code.sub(rsp, 8); \
			code.mov(dword[rsp + 32], cell); \
			code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, int, int)>(x)); \
			code.call(rax); \
			code.add(rsp, 8 + 8);

			case FUNC_GRAIN:
				CALLFUNC4M(grain);
				break;
			case FUNC_SIDFILTER:
				CALLFUNC4M(sidfilter);
				break;

#define CALLFUNC0M(x) \
			stack++; \
			store_xmm0(code, first); \
			code.mov(rcx, cell); /* set up memory params */ \
			code.mov(rdx, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(int,int)>(x)); \
			code.call(rax);

			case FUNC_BUTTON:
				CALLFUNC0M(button);
				break;

#define CALLFUNC0MC(x,c) \
			stack++; \
			store_xmm0(code, first); \
			code.mov(rax, (size_t)c); /* store constant */ \
			code.movq(xmm0, rax); \
			code.mov(rdx, cell); /* set up memory params */ \
			code.mov(r8, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(double,int,int)>(x)); \
			code.call(rax);

			case FUNC_TOGGLE:
				CALLFUNC0MC(toggle, 0);
				break;

			case FUNC_SLIDER:
				CALLFUNC0MC(slider, 0);
				break;
			case FUNC_ENCODER:
				CALLFUNC0MC(encoder, 0);
				break;

#define CALLFUNC1MC2(x,c) \
			if (c) { \
			code.mov(rax, c); /* store constant */ \
			code.movq(xmm1, rax); \
			} else { \
			code.xorpd(xmm1, xmm1); \
			} \
			code.mov(r8, cell); /* set up memory params */ \
			code.mov(r9, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(double,double,int,int)>(x)); \
			code.call(rax);

			case FUNC_TRIGGER:
			{
				double_64int t;
				t.f = 0;
				CALLFUNC1MC2(trigger, t.i64);
			}
			break;

#define CALLFUNC1MC(x,c) \
			code.movsd(xmm1, xmm0); \
			code.mov(rax, (size_t)c); /* store constant */ \
			code.movq(xmm0, rax); \
			code.mov(r8, cell); /* set up memory params */ \
			code.mov(r9, memofs); \
			code.mov(rax, (size_t)static_cast<double(*)(double,double,int,int)>(x)); \
			code.call(rax); \

			case FUNC_MIDIPOT:
			{
				double_64int t;
				t.f = 0;
				CALLFUNC1MC(midipot, t.i64);
			}
			break;
			case FUNC_SLIDERPOT:
			{
				double_64int t;
				t.f = 0;
				CALLFUNC1MC(sliderpot, t.i64);
			}
			break;
			case FUNC_TOGGLEPOT:
			{
				double_64int t;
				t.f = 0;
				CALLFUNC1MC(togglepot, t.i64);
			}
			break;

#define CALLFUNC2C(x,c) \
			stack--; \
			code.movsd(xmm1, xmm0); \
			code.mov(rax, (size_t)c); /* store constant */ \
			code.movq(xmm2, rax); \
			code.movq(xmm0, qword[rsp+32]); /* pop from stack */ \
			code.add(rsp, 16); \
			code.mov(rax, (size_t)static_cast<double(*)(double, double, double)>(x)); \
			code.call(rax);

			case FUNC_B_ROTLEFT:
			{
				double_64int t;
				t.f = 32;
				CALLFUNC2C(b_rotleft, t.i64);
			}
			break;
			case FUNC_B_ROTRIGHT:
			{
				double_64int t;
				t.f = 32;
				CALLFUNC2C(b_rotright, t.i64);
			}
			break;
			case FUNC_HILIGHT:
			{
				double_64int t;
				t.f = rgb(0.1f, 0.3f, 0.1f);
				CALLFUNC2C(hilight, t.i64);
			}
			break;

			case FUNC_DEGREES:
			{
				double_64int t;
				t.f = (180 / M_PI);
				code.mov(rax, t.i64);
				code.movq(xmm1, rax);
				code.mulsd(xmm0, xmm1);
			}
			break;
			case FUNC_RADIANS:
			{
				double_64int t;
				t.f = (M_PI / 180);
				code.mov(rax, t.i64);
				code.movq(xmm1, rax);
				code.mulsd(xmm0, xmm1);
			}
			break;
			case FUNC_ISNAN:
				code.mov(rax, one.i64);
				code.movq(xmm1, rax);
				code.cmppd(xmm0, xmm0, 3); // check for nan:s, result all 0 or all 1
				code.andpd(xmm0, xmm1); // and mask with pattern of 1.0				
				break;
			case FUNC_NANKILL:
				code.movq(xmm1, xmm0);
				code.cmppd(xmm0, xmm0, 7); // check for not nan:s, result all 0 or all 1
				code.andpd(xmm0, xmm1); // and mask with pattern of original value				
				break;

			case FUNC_IF:
				stack -= 2;
				code.movq(xmm2, xmm0);
				code.movq(xmm1, qword[rsp + 32]); // pop from stack 
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); // pop from stack 
				code.add(rsp, 16);
				code.mov(rax, epsilon.i64);
				code.movq(xmm3, rax);
				code.cmppd(xmm0, xmm3, 5); // xmm0 > xmm3; xmm0 is all 0:s or all 1:s
				code.andpd(xmm1, xmm0);    // xmm1 is 0 or 'then'
				code.andnpd(xmm0, xmm2);   // xmm0 is 0 or 'else'
				code.orpd(xmm0, xmm1);     // xmm0 is 'then' or 'else'				
				break;

			case FUNC_AND:
				stack--; // params are swapped but that doesn't matter
				code.movq(xmm1, qword[rsp + 32]); // pop from stack 
				code.add(rsp, 16);
				code.mov(rax, epsilon.i64);
				code.movq(xmm3, rax);
				code.cmppd(xmm0, xmm3, 5); // all 0:s or all 1:s
				code.cmppd(xmm1, xmm3, 5); // all 0:s or all 1:s
				code.andpd(xmm0, xmm1);
				code.mov(rax, one.i64); // finally and with pattern of 1.0
				code.movq(xmm3, rax);
				code.andpd(xmm0, xmm3);
				break;
			case FUNC_OR:
				stack--; // params are swapped but that doesn't matter
				code.movq(xmm1, qword[rsp + 32]); // pop from stack 
				code.add(rsp, 16);
				code.mov(rax, epsilon.i64);
				code.movq(xmm3, rax);
				code.cmppd(xmm0, xmm3, 5); // all 0:s or all 1:s
				code.cmppd(xmm1, xmm3, 5); // all 0:s or all 1:s
				code.orpd(xmm0, xmm1);
				code.mov(rax, one.i64); // finally and with pattern of 1.0
				code.movq(xmm3, rax);
				code.andpd(xmm0, xmm3);
				break;
			case FUNC_XOR:
				stack--; // params are swapped but that doesn't matter
				code.movq(xmm1, qword[rsp + 32]); // pop from stack 
				code.add(rsp, 16);
				code.mov(rax, epsilon.i64);
				code.movq(xmm3, rax);
				code.cmppd(xmm0, xmm3, 5); // all 0:s or all 1:s
				code.cmppd(xmm1, xmm3, 5); // all 0:s or all 1:s
				code.xorpd(xmm0, xmm1);
				code.mov(rax, one.i64); // finally and with pattern of 1.0
				code.movq(xmm3, rax);
				code.andpd(xmm0, xmm3);
				break;
			case FUNC_NOT:
				code.mov(rax, epsilon.i64);
				code.movq(xmm3, rax);
				code.cmppd(xmm0, xmm3, 1); // all 0:s or all 1:s; comparison is swapped
				code.mov(rax, one.i64); // finally and with pattern of 1.0
				code.movq(xmm3, rax);
				code.andpd(xmm0, xmm3);
				break;

			case FUNC_PROBE:
				code.movsd(xmm1, xmm0);
				code.xorpd(xmm0, xmm0);
				code.mov(rax, (size_t)static_cast<double(*)(double, double)>(probe));
				code.call(rax);
				break;

			case FUNC_ADSR:
				stack -= 4;
				code.movsd(xmm5, xmm0);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm5 has 5th.
				code.sub(rsp, 8); // 16 byte align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 5th to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(adsr));
				code.call(rax);
				code.add(rsp, 8 + 8 * 2 + 8); // 5th, cell, memofs, align off stack
				break;

			case FUNC_CATMULLROM:
				stack -= 4;
				code.movsd(xmm5, xmm0);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm5 has 5th.
				code.sub(rsp, 8); // 16 byte align
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 5th to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double)>(catmullrom));
				code.call(rax);
				code.add(rsp, 8 + 8); // 5th, cell, memofs, align off stack
				break;

			case FUNC_HEXWAVE:
				stack -= 4;
				code.movsd(xmm5, xmm0);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm5 has 5th.
				code.sub(rsp, 8); // 16 byte align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 5th to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(hexwave));
				code.call(rax);
				code.add(rsp, 8 + 8 * 2 + 8); // 5th, cell, memofs, align off stack
				break;


			case FUNC_ALLPASS:
				stack -= 2;
				code.movsd(xmm2, xmm0);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.mov(r9, cell);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8); // 16 byte align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, int, int)>(allpass));
				code.call(rax);
				code.add(rsp, 8 + 8); // memofs, align off stack
				break;

			case FUNC_COMB:
				stack -= 3;
				code.movsd(xmm3, xmm0);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, int, int)>(comb));
				code.call(rax);
				code.add(rsp, 8 + 8); // memofs, cell off stack
				break;

			case FUNC_REVERB:
				stack -= 3;
				code.movsd(xmm3, xmm0);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, int, int)>(reverb));
				code.call(rax);
				code.add(rsp, 8 + 8); // memofs, cell off stack
				break;

			case FUNC_FILTER:
				stack -= 3;
				code.movsd(xmm3, xmm0);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2 and 3 have fist 4 params, count, cell and memofs goes to stack
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(rax, one.i64);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // align, count, memofs, cell off stack
				break;

			case FUNC_LPF:
				stack -= 2;
				code.movsd(xmm2, xmm0);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.xorpd(xmm3, xmm3);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(rax, one.i64);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;

			case FUNC_HPF:
				stack -= 2;
				code.movsd(xmm2, xmm0);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.mov(rax, one.i64);
				code.movq(xmm3, rax);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(rax, one.i64);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;

			case FUNC_BPF:
				stack -= 2;
				code.movsd(xmm2, xmm0);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				{
					double_64int t;
					t.f = 2.0;
					code.mov(rax, t.i64);
				}
				code.movq(xmm3, rax);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8);
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(rax, one.i64);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;

			case FUNC_MAP:
				stack -= 4;
				code.movsd(xmm5, xmm0);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm5 has 5th.
				code.sub(rsp, 8); // 16 byte align
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 5th to stack
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double)>(jit_kludge::map));
				code.call(rax);
				code.add(rsp, 8 + 8); // 5th, align off stack
				break;

#define CALLFUNC5M(x) \
				stack -= 4; \
				code.movq(rax, xmm0); \
				code.movq(xmm3, qword[rsp + 32]);  \
				code.add(rsp, 16); \
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */ \
				code.add(rsp, 16); \
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */ \
				code.add(rsp, 16); \
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */ \
				code.add(rsp, 16);				 \
				code.sub(rsp, 8); /* align*/ \
				code.sub(rsp, 8); \
				code.mov(dword[rsp + 32], memofs);  /* push memofs to stack*/ \
				code.sub(rsp, 8); \
				code.mov(dword[rsp + 32], cell);  /* push cell to stack*/ \
				code.sub(rsp, 8); \
				code.mov(qword[rsp + 32], rax);  \
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(x)); \
				code.call(rax); \
				code.add(rsp, 8 + 8 + 8 + 8);

			case FUNC_FILTERC:
				CALLFUNC5M(filter);
				break;
			case FUNC_PLAYLOOPP:
				CALLFUNC5M(playloopp);
				break;
			case FUNC_PLAYLOOPX:
				CALLFUNC5M(playloopx);
				break;
			case FUNC_SIDENVELOPE:
				CALLFUNC5M(sidenvelope);
				break;

			case FUNC_LPFC:
				stack -= 3;
				code.movq(rax, xmm0);
				code.movq(xmm2, qword[rsp + 32]);
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.xorpd(xmm3, xmm3);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;

			case FUNC_HPFC:
				stack -= 3;
				code.movq(rax, xmm0);
				code.movq(xmm2, qword[rsp + 32]);
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.mov(rbx, one.i64);
				code.movq(xmm3, rbx);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;

			case FUNC_BPFC:
				stack -= 3;
				code.movq(rax, xmm0);
				code.movq(xmm2, qword[rsp + 32]);
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				{
					double_64int t;
					t.f = 2.0;
					code.mov(rbx, t.i64);
				}
				code.movq(xmm3, rbx);
				// At this point xmm0,1,2 and r9 have fist 4 params, memofs is the last and goes to stack
				code.sub(rsp, 8);
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.mov(qword[rsp + 32], rax);
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, int, int)>(filter));
				code.call(rax);
				code.add(rsp, 8 + 8 + 8 + 8); // memofs, cell off stack
				break;


			case FUNC_PARANOISE:
				stack -= 9; // what was I thinking..
				code.movsd(xmm9, xmm0);
				code.movq(xmm8, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm7, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm6, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm5, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm4, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm4-9 has 5th-10th.
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm9); // push 10
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm8); // push 9
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm7); // push 8
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm6); // push 7
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 6
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm4); // push 5
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, double, double, double, double, double, int, int)>(paranoise));
				code.call(rax);
				code.add(rsp, 8 * 8); // 5-10th, memofs and cell off stack
				break;

			case FUNC_PADSYNTH:
				stack -= 10;
				code.movsd(xmm10, xmm0);
				code.movq(xmm9, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm8, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm7, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm6, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm5, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm4, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm4-10 has 5th-11th.
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm10); // push 11
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm9); // push 10
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm8); // push 9
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm7); // push 8
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm6); // push 7
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 6
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm4); // push 5
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, double, double, double, double, double, double, int, int)>(padsynth));
				code.call(rax);
				code.add(rsp, 10 * 8); // align, 5-11th, memofs and cell off stack
				break;

			case FUNC_PADSYNTH22:
				stack -= 10;
				code.movsd(xmm10, xmm0);
				code.movq(xmm9, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm8, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm7, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm6, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm5, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm4, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm4-10 has 5th-11th.
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm10); // push 11
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm9); // push 10
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm8); // push 9
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm7); // push 8
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm6); // push 7
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 6
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm4); // push 5
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, double, double, double, double, double, double, int, int)>(padsynth22));
				code.call(rax);
				code.add(rsp, 10 * 8); // align, 5-11th, memofs and cell off stack
				break;

			case FUNC_AY:
				stack -= 10;
				code.movsd(xmm10, xmm0);
				code.movq(xmm9, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm8, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm7, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm6, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm5, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm4, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm4-10 has 5th-11th.
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm10); // push 11
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm9); // push 10
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm8); // push 9
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm7); // push 8
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm6); // push 7
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 6
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm4); // push 5
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, double, double, double, double, double, double, int, int)>(ay));
				code.call(rax);
				code.add(rsp, 10 * 8); // align, 5-11th, memofs and cell off stack
				break;
			case FUNC_BIQUAD:
				stack -= 6;
				code.movsd(xmm6, xmm0);
				code.movq(xmm5, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm4, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm3, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm2, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm1, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				code.movq(xmm0, qword[rsp + 32]); /* pop from stack */
				code.add(rsp, 16);
				// At this point xmm0,1,2,3 have fist 4 params and xmm4-6 has 5th-7th.
				code.sub(rsp, 8); // align
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], memofs);  // push memofs to stack
				code.sub(rsp, 8);
				code.mov(dword[rsp + 32], cell);  // push cell to stack
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm6); // push 7
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm5); // push 6
				code.sub(rsp, 8);
				code.movq(qword[rsp + 32], xmm4); // push 5
				code.mov(rax, (size_t)static_cast<double(*)(double, double, double, double, double, double, double, int, int)>(biquad));
				code.call(rax);
				code.add(rsp, 6 * 8); // align, 5-7th, memofs and cell off stack
				break;

			case FUNC_PINKNOISE:
				CALLFUNC0M(jit_kludge::pinknoise);
				break;
			case FUNC_BROWNNOISE:
				CALLFUNC0M(jit_kludge::brownnoise);
				break;
			case FUNC_BLUENOISE:
				CALLFUNC0M(jit_kludge::bluenoise);
				break;
			case FUNC_NOP:
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP1:
				stack--;
				code.add(rsp, 16);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP2:
				stack -= 2;
				code.add(rsp, 16*2);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP3:
				stack -= 3;
				code.add(rsp, 16*3);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP4:
				stack -= 4;
				code.add(rsp, 16*4);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP5:
				stack -= 5;
				code.add(rsp, 16*5);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP6:
				stack -= 6;
				code.add(rsp, 16*6);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP7:
				stack -= 7;
				code.add(rsp, 16*7);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP8:
				stack -= 8;
				code.add(rsp, 16*8);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP9:
				stack -= 9;
				code.add(rsp, 16*9);
				PUSH_CONST(0.0);
				break;
			case FUNC_NOP10:
				stack -= 10;
				code.add(rsp, 16*10);
				PUSH_CONST(0.0);
				break;

			default:
				return 1;
			}
			memofs += gFunc[func].mMemory + gFunc[func].mSamplerateMemory * gSamplerate;
		}
		break;

		default:
			return 1;
		}
	}
	if (stack != 1)
		return 1;

	// last item is in xmm0, write out
	code.movq(qword[r15 + cell * 8], xmm0);
	return 0;
}

jitfunc jit_finish(Xbyak::CodeGenerator& code)
{
	using namespace Xbyak::util;

	code.add(rsp, 32); // undo function call stack trick

	code.pop(r15);
	code.pop(r14);
	code.pop(r13);
	code.pop(r12);
	code.pop(rdi);
	code.pop(rbp);
	code.pop(rbx);
	code.movdqu(xmm6, xword[rsp + 16 * 0]);
	code.movdqu(xmm7, xword[rsp + 16 * 1]);
	code.movdqu(xmm8, xword[rsp + 16 * 2]);
	code.movdqu(xmm9, xword[rsp + 16 * 3]);
	code.movdqu(xmm10, xword[rsp + 16 * 4]);
	code.movdqu(xmm11, xword[rsp + 16 * 5]);
	code.movdqu(xmm12, xword[rsp + 16 * 6]);
	code.movdqu(xmm13, xword[rsp + 16 * 7]);
	code.movdqu(xmm14, xword[rsp + 16 * 8]);
	code.movdqu(xmm15, xword[rsp + 16 * 9]);
	code.add(rsp, 16 * 10);
	code.ret();
	code.readyRE();

	return code.getCode<double(*)(double*, double*)>();
}

jitfunc jit(Op* bc, int cell, Xbyak::CodeGenerator &code, char *stringstore)
{
	jit_init(code);
	if (jit_addcode(bc, cell, code, stringstore))
		return 0;
	return jit_finish(code);
}
