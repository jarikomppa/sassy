#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

using namespace EvalFunc;

double compute(Op* bc, int cell, char*stringstore)
{
	int memofs = 0;
	double stack[256];
	int sp = 0;
	if (bc->mOpcode == 0)
		return nan("0");
	stack[0] = nan("0");
	while (bc->mOpcode)
	{
		switch (bc->mOpcode)
		{
		case 'C': // numeric constant
		{
			stack[sp] = bc->mArg.f; sp++;
			bc++;
		}
		break;
		case 'T': // text
		{
			stack[sp] = (double)bc->mArg.i.i; sp++;
			bc++;
		}
		break;
		case 'V': // variable
		{
			stack[sp] = getvar(bc->mArg.i.i); sp++;
			bc++;
		}
		break;
		case 'A': // area
		{
			stack[sp] = bc->mArg.i.i; sp++;
			stack[sp] = bc->mArg.i.j; sp++;
			bc++;
		}
		break;
		case 'F': // function call
		{
			int func = bc->mArg.i.i;
			bc++;
			switch (func)
			{
			case FUNC_SIN:
				stack[sp - 1] = sin(stack[sp - 1]);
				break;
			case FUNC_IF:
				sp -= 2;
				if (fabs(stack[sp - 1]) > 0.0001)
					stack[sp - 1] = stack[sp];
				else
					stack[sp - 1] = stack[sp + 1];
				break;
			case FUNC_TIME:
				stack[sp] = gettime();
				sp++;
				break;
			case FUNC_OUT:
				stack[sp - 1] = output(stack[sp - 1]);
				break;
			case FUNC_OUT2:
				sp--;
				stack[sp - 1] = output2(stack[sp - 1], stack[sp]);
				break;
			case FUNC_PI:
				stack[sp] = M_PI;
				sp++;
				break;
			case FUNC_TRUNC:
				stack[sp - 1] = trunc(stack[sp - 1]);
				break;
			case FUNC_FRACT:
				stack[sp - 1] = stack[sp - 1] - trunc(stack[sp - 1]);
				break;
			case FUNC_MOD:
				sp--;
				stack[sp - 1] = fmod(stack[sp - 1], stack[sp]);
				break;
			case FUNC_GRAPH:
				stack[sp - 1] = graph(stack[sp - 1], cell, memofs);
				break;
			case FUNC_SLIDER:
				stack[sp] = slider(0, cell, memofs);
				sp++;
				break;
			case FUNC_MIDIPOT:
				stack[sp - 1] = midipot(0, stack[sp - 1], cell, memofs);
				break;
			case FUNC_MIDIVAL:
				stack[sp] = midival();
				sp++;
				break;
			case FUNC_MIDINOTE:
				stack[sp] = midinote();
				sp++;
				break;
			case FUNC_MIDIVEL:
				stack[sp] = midivel();
				sp++;
				break;
			case FUNC_MIDION:
				stack[sp] = midion();
				sp++;
				break;
			case FUNC_MIDIPITCH:
				stack[sp] = midipitch();
				sp++;
				break;
			case FUNC_MIDIPROG:
				stack[sp] = midiprog();
				sp++;
				break;
			case FUNC_DELAY:
				sp--;
				stack[sp - 1] = delay(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_NOISE:
				stack[sp] = noise();
				sp++;
				break;
			case FUNC_SIN1:
				stack[sp - 1] = sin1(stack[sp - 1]);
				break;
			case FUNC_TRIANGLE:
				stack[sp - 1] = triangle(stack[sp - 1]);
				break;
			case FUNC_SQUARE:
				stack[sp - 1] = square(stack[sp - 1]);
				break;
			case FUNC_SQUAREQ:
				stack[sp - 1] = squareq(stack[sp - 1]);
				break;
			case FUNC_SAW:
				stack[sp - 1] = saw(stack[sp - 1]);
				break;
			case FUNC_SAWQ:
				stack[sp - 1] = sawq(stack[sp - 1]);
				break;
			case FUNC_PULSE:
				sp--;
				stack[sp - 1] = pulse(stack[sp - 1], stack[sp]);
				break;
			case FUNC_HOLD:
				sp--;
				stack[sp - 1] = hold(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_QUANTIZE:
				{
					double t = 12 * log(32 * POW_2_3_4TH * (stack[sp - 1]) / 440) / log(2);
					int i = (int)t;
					if (i >= 0 && i < 128)
						stack[sp - 1] = note_to_freq[i];
				}
				break;
			case FUNC_SQRT:
				stack[sp - 1] = sqrt(stack[sp - 1]);
				break;
			case FUNC_LOG:
				stack[sp - 1] = log((float)stack[sp - 1]);
				break;
			case FUNC_POW:
				sp--;
				stack[sp - 1] = pow(stack[sp - 1], stack[sp]);
				break;
			case FUNC_COS:
				stack[sp - 1] = cos(stack[sp - 1]);
				break;
			case FUNC_TAN:
				stack[sp - 1] = tan((float)stack[sp - 1]);
				break;
			case FUNC_ACOS:
				stack[sp - 1] = acos(stack[sp - 1]);
				break;
			case FUNC_ASIN:
				stack[sp - 1] = asin(stack[sp - 1]);
				break;
			case FUNC_ATAN:
				stack[sp - 1] = atan(stack[sp - 1]);
				break;
			case FUNC_ATAN2:
				sp--;
				stack[sp - 1] = atan2(stack[sp - 1], stack[sp]);
				break;
			case FUNC_COSH:
				stack[sp - 1] = cosh(stack[sp - 1]);
				break;
			case FUNC_SINH:
				stack[sp - 1] = sinh(stack[sp - 1]);
				break;
			case FUNC_TANH:
				stack[sp - 1] = tanh(stack[sp - 1]);
				break;
			case FUNC_EXP:
				stack[sp - 1] = exp(stack[sp - 1]);
				break;
			case FUNC_LOG10:
				stack[sp - 1] = log10((float)stack[sp - 1]);
				break;
			case FUNC_FLOOR:
				stack[sp - 1] = floor(stack[sp - 1]);
				break;
			case FUNC_ABS:
				stack[sp - 1] = fabs(stack[sp - 1]);
				break;
			case FUNC_ADSR:
				sp -= 4;
				stack[sp - 1] = adsr(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;
			case FUNC_BUTTON:
				stack[sp] = button(cell, memofs);
				sp++;
				break;
			case FUNC_TOGGLE:
				stack[sp] = toggle(0, cell, memofs);
				sp++;
				break;
			case FUNC_FILTER:
				sp -= 3;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], 1, cell, memofs);
				break;
			case FUNC_LPF:
				sp -= 2;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 0, 1, cell, memofs);
				break;
			case FUNC_HPF:
				sp -= 2;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 1, 1, cell, memofs);
				break;
			case FUNC_BPF:
				sp -= 2;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 2, 1, cell, memofs);
				break;
			case FUNC_FFT:
				stack[sp - 1] = fft(stack[sp - 1], cell, memofs);
				break;
			case FUNC_NOTETOFREQ:
				if ((int)stack[sp - 1] >= 0 && (int)stack[sp - 1] < 128)
					stack[sp - 1] = note_to_freq[(int)stack[sp - 1]];
				else
					stack[sp - 1] = 0;
				break;
			case FUNC_FREQTONOTE:
				stack[sp - 1] = 12 * log(32 * POW_2_3_4TH * (stack[sp - 1] / 440)) / log(2);
				break;
			case FUNC_LATCH:
				sp--;
				stack[sp - 1] = latch(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_AREAMIN:
				sp--;
				stack[sp - 1] = tablemin((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_AREAMAX:
				sp--;
				stack[sp - 1] = tablemax((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_AVERAGE:
				sp--;
				stack[sp - 1] = tableaverage((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_SUM:
				sp--;
				stack[sp - 1] = tablesum((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_SELECT:
				sp-=2;
				stack[sp - 1] = tableselect((int)stack[sp - 1], (int)stack[sp], stack[sp+1]);
				break;
			case FUNC_SELECTV:
				sp -= 2;
				stack[sp - 1] = tableselectv((int)stack[sp - 1], (int)stack[sp], stack[sp + 1]);
				break;
			case FUNC_ISNAN:
				stack[sp - 1] = isnormal(stack[sp - 1]) ? 0 : 1;
				break;
			case FUNC_NAN:
				stack[sp] = nan("0");
				sp++;
				break;
			case FUNC_TIMEGATE:
				stack[sp - 1] = timegate(stack[sp - 1], cell, memofs);
				break;
			case FUNC_NANKILL:
				stack[sp - 1] = !isnormal(stack[sp - 1]) ? 0 : stack[sp - 1];
				break;
			case FUNC_SUPERSAW:
				sp -= 2;
				stack[sp - 1] = supersaw(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_SUPERSQUARE:
				sp -= 2;
				stack[sp - 1] = supersquare(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_SUPERSIN:
				sp -= 2;
				stack[sp - 1] = supersin(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_SQUAREF:
				sp--;
				stack[sp - 1] = squaref(stack[sp - 1], stack[sp]);
				break;
			case FUNC_SAWF:
				sp--;
				stack[sp - 1] = sawf(stack[sp - 1], stack[sp]);
				break;
			case FUNC_SQUARESAW:
				sp--;
				stack[sp - 1] = squaresaw(stack[sp - 1], stack[sp]);
				break;
			case FUNC_AND:
				sp--;
				stack[sp - 1] = (isnan(stack[sp - 1]) || isnan(stack[sp])) ? 1 : ((stack[sp - 1] > 0.0001) && (stack[sp] > 0.0001)) ? 1.0 : 0.0;
				break;
			case FUNC_OR:
				sp--;
				stack[sp - 1] = ((stack[sp - 1] > 0.0001) || (stack[sp] > 0.0001)) ? 1.0 : 0.0;
				break;
			case FUNC_NOT:
				stack[sp - 1] = (!(stack[sp - 1] > 0.0001)) ? 1.0 : 0.0;
				break;
			case FUNC_XOR:
				sp--;
				stack[sp - 1] = ((stack[sp - 1] > 0.0001) ^ (stack[sp] > 0.0001)) ? 1.0 : 0.0;
				break;
			case FUNC_TRUE:
				stack[sp] = 1.0;
				sp++;
				break;
			case FUNC_FALSE:
				stack[sp] = 0.0;
				sp++;
				break;
			case FUNC_DEGREES:
				stack[sp - 1] = stack[sp - 1] * (180 / M_PI);
				break;
			case FUNC_RADIANS:
				stack[sp - 1] = stack[sp - 1] * (M_PI / 180);
				break;
			case FUNC_EVEN:
				stack[sp - 1] = ((int)stack[sp - 1] & 1) ? 0.0 : 1.0;
				break;
			case FUNC_ODD:
				stack[sp - 1] = ((int)stack[sp - 1] & 1) ? 1.0 : 0.0;
				break;
			case FUNC_SIGN:
				stack[sp - 1] = (stack[sp - 1] < 0.0) ? -1.0 : 1.0;
				break;
			case FUNC_BAR:
				stack[sp - 1] = bar(stack[sp - 1], cell, memofs);
				break;
			case FUNC_PIE:
				stack[sp - 1] = pie(stack[sp - 1], cell, memofs);
				break;
			case FUNC_MIX:
				sp -= 2;
				stack[sp - 1] = stack[sp] + stack[sp - 1] * (stack[sp + 1] - stack[sp]);
				break;
			case FUNC_SMOOTHSTEP:
				stack[sp - 1] = stack[sp - 1] * stack[sp - 1] * (3 - 2 * stack[sp - 1]);
				break;
			case FUNC_SMOOTHERSTEP:
				stack[sp - 1] = stack[sp - 1] * stack[sp - 1] * stack[sp - 1] * (stack[sp - 1] * (stack[sp - 1] * 6 - 15) + 10);
				break;
			case FUNC_MIN:
				sp--;
				stack[sp - 1] = (stack[sp - 1] > stack[sp]) ? stack[sp] : stack[sp - 1];
				break;
			case FUNC_MAX:
				sp--;
				stack[sp - 1] = (stack[sp - 1] < stack[sp]) ? stack[sp] : stack[sp - 1];
				break;
			case FUNC_CLAMP:
				sp -= 2;
				stack[sp - 1] = (stack[sp - 1] < stack[sp]) ? stack[sp] : (stack[sp - 1] > stack[sp + 1]) ? stack[sp + 1] : stack[sp - 1];
				break;
			case FUNC_MAP:
				// v a->b c->d
				sp -= 4;
				stack[sp - 1] = ((stack[sp - 1] - stack[sp]) / (stack[sp + 1] - stack[sp])) * (stack[sp + 3] - stack[sp + 2]) + stack[sp + 2];
				break;
			case FUNC_COUNT:
				sp--;
				stack[sp - 1] = tablecount((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_FIND:
				sp -= 2;
				stack[sp - 1] = tablefind((int)stack[sp - 1], (int)stack[sp], stack[sp + 1]);
				break;
			case FUNC_FINDV:
				sp -= 2;
				stack[sp - 1] = tablefindv((int)stack[sp - 1], (int)stack[sp], stack[sp + 1]);
				break;
			case FUNC_PRODUCT:
				sp--;
				stack[sp - 1] = tableproduct((int)stack[sp - 1], (int)stack[sp]);
				break;
			case FUNC_HILIGHT:
				sp--;
				stack[sp - 1] = hilight(stack[sp - 1], stack[sp], rgb(0.1f, 0.3f, 0.1f));
				break;
			case FUNC_REPLACE:
				sp-=2;
				stack[sp - 1] = replace(stack[sp - 1], stack[sp], stack[sp+1]);
				break;
			case FUNC_LOOKUP:
				sp--;
				stack[sp - 1] = lookup(stack[sp - 1], stack[sp]);
				break;
			case FUNC_DT:
				stack[sp] = dt();
				sp++;
				break;
			case FUNC_STEP:
				stack[sp - 1] = step(stack[sp - 1], cell, memofs);
				break;
			case FUNC_ALLPASS:
				sp -= 2;
				stack[sp - 1] = allpass(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_COMB:
				sp -= 3;
				stack[sp - 1] = comb(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], cell, memofs);
				break;
			case FUNC_REVERB:
				sp -= 3;
				stack[sp - 1] = reverb(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], cell, memofs);
				break;
			case FUNC_PINKNOISE:
				stack[sp] = paranoise(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, cell, memofs);
				sp++;
				break;
			case FUNC_BROWNNOISE:
				stack[sp] = paranoise(10, 9, 8, 7, 6, 5, 4, 3, 2, 1, cell, memofs);
				sp++;
				break;
			case FUNC_BLUENOISE:
				stack[sp] = paranoise(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, cell, memofs);
				sp++;
				break;
			case FUNC_PARANOISE:
				sp -= 9;
				stack[sp - 1] = paranoise(stack[sp-1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], stack[sp + 4], stack[sp + 5], stack[sp + 6], stack[sp + 7], stack[sp + 8], cell, memofs);
				break;
			case FUNC_OPL1:
				stack[sp - 1] = opl1(stack[sp - 1]);
				break;
			case FUNC_OPL2:
				stack[sp - 1] = opl2(stack[sp - 1]);
				break;
			case FUNC_OPL3:
				stack[sp - 1] = opl3(stack[sp - 1]);
				break;
			case FUNC_SLIDERV:
				stack[sp - 1] = slider(stack[sp - 1], cell, memofs);
				break;
			case FUNC_MIDIPOTV:
				sp--;
				stack[sp - 1] = midipot(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_TOGGLEV:
				stack[sp - 1] = toggle(stack[sp - 1], cell, memofs);				
				break;
			case FUNC_NOTETOFREQSLOW:
				stack[sp - 1] = pow(2, (stack[sp - 1] - 69) / 12.0) * 440;
				break;
			case FUNC_TRIGGER:
				stack[sp - 1] = trigger(stack[sp - 1], 0, cell, memofs);
				break;
			case FUNC_TRIGGERM:
				sp--;
				stack[sp - 1] = trigger(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_B_AND:
				sp--;
				stack[sp - 1] = b_and(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_OR:
				sp--;
				stack[sp - 1] = b_or(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_NOT:
				stack[sp - 1] = b_not(stack[sp - 1]);
				break;
			case FUNC_B_XOR:
				sp--;
				stack[sp - 1] = b_xor(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_NAND:
				sp--;
				stack[sp - 1] = b_nand(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_NOR:
				sp--;
				stack[sp - 1] = b_nor(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_SHLEFT:
				sp--;
				stack[sp - 1] = b_shleft(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_SHRIGHT:
				sp--;
				stack[sp - 1] = b_shright(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_ROTLEFT:
				sp--;
				stack[sp - 1] = b_rotleft(stack[sp - 1], stack[sp], 32);
				break;
			case FUNC_B_ROTLEFTV:
				sp-=2;
				stack[sp - 1] = b_rotleft(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_B_ROTRIGHT:
				sp--;
				stack[sp - 1] = b_rotright(stack[sp - 1], stack[sp], 32);
				break;
			case FUNC_B_ROTRIGHTV:
				sp-=2;
				stack[sp - 1] = b_rotright(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_B_TEST:
				sp--;
				stack[sp - 1] = b_test(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_SET:
				sp--;
				stack[sp - 1] = b_set(stack[sp - 1], stack[sp]);
				break;
			case FUNC_B_CLEAR:
				sp--;
				stack[sp - 1] = b_clear(stack[sp - 1], stack[sp]);
				break;
			case FUNC_HILIGHTV:
				sp-=2;
				stack[sp - 1] = hilight(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_RGB:
				sp -= 2;
				stack[sp - 1] = rgb(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_MIDIOUT:
				sp--;
				stack[sp - 1] = midiout(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MIDIOUTPITCH:
				stack[sp - 1] = midioutpitch(stack[sp - 1]);
				break;
			case FUNC_MIDIOUTPOT:
				sp--;
				stack[sp - 1] = midioutpot(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MIDIOUTPROG:
				stack[sp - 1] = midioutprog(stack[sp - 1]);
				break;
			case FUNC_MIDIOUTRAW:
				sp -= 3;
				stack[sp - 1] = midioutraw(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2]);
				break;
			case FUNC_RUBBERBAND:
				sp--;
				stack[sp - 1] = rubberband(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_SLIDERPOT:
				stack[sp - 1] = sliderpot(0, stack[sp - 1], cell, memofs);
				break;
			case FUNC_SLIDERPOTV:
				sp--;
				stack[sp - 1] = sliderpot(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_IN:
				sp++;
				stack[sp - 1] = in();
				break;
			case FUNC_IN2:
				stack[sp - 1] = in2(stack[sp - 1]);
				break;
			case FUNC_DC:
				stack[sp - 1] = dc(stack[sp - 1], cell, memofs);
				break;
			case FUNC_DRUNKARDSWALK:
				sp--;
				stack[sp - 1] = drunkardswalk(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_TOGGLEPOT:
				stack[sp - 1] = togglepot(0, stack[sp - 1], cell, memofs);
				break;
			case FUNC_TOGGLEPOTV:
				sp--;
				stack[sp - 1] = togglepot(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_CPU:
				sp++;
				stack[sp - 1] = cpu();
				break;
			case FUNC_MIDIVALV:
				stack[sp - 1] = midivalv(stack[sp - 1]);
				break;
			case FUNC_MIDINOTEV:
				stack[sp - 1] = midinotev(stack[sp - 1]);
				break;
			case FUNC_MIDIVELV:
				stack[sp - 1] = midivelv(stack[sp - 1]);
				break;
			case FUNC_MIDIONV:
				stack[sp - 1] = midionv(stack[sp - 1]);
				break;
			case FUNC_STEPG:
				sp--;
				stack[sp - 1] = stepg(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_PLOTXY:
				stack[sp - 1] = plotxy(stack[sp - 1], cell, memofs);
				break;
			case FUNC_PLOTXY2:
				sp--;
				stack[sp - 1] = plotxy2(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_HOLDL:
				sp-=2;
				stack[sp - 1] = holdl(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_PROBE:
				stack[sp - 1] = probe(0, stack[sp - 1]);
				break;
			case FUNC_PROBEC:
				sp--;
				stack[sp - 1] = probe(stack[sp - 1], stack[sp]);
				break;
			case FUNC_BITCRUNCH:
				sp--;
				stack[sp - 1] = bitcrunch(stack[sp - 1], stack[sp]);
				break;
			case FUNC_FILTERC:
				sp -= 4;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;
			case FUNC_LPFC:
				sp -= 3;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 0, stack[sp + 2], cell, memofs);
				break;
			case FUNC_HPFC:
				sp -= 3;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 1, stack[sp + 2], cell, memofs);
				break;
			case FUNC_BPFC:
				sp -= 3;
				stack[sp - 1] = filter(stack[sp - 1], stack[sp], stack[sp + 1], 2, stack[sp + 2], cell, memofs);
				break;
			case FUNC_MIDICHANNEL:
				stack[sp - 1] = midichannel(stack[sp - 1]);
				break;
			case FUNC_MIDIVALC:
				sp--;
				stack[sp - 1] = midivalc(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MIDINOTEC:
				sp--;
				stack[sp - 1] = midinotec(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MIDIVELC:
				sp--;
				stack[sp - 1] = midivelc(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MICIONC:
				sp--;
				stack[sp - 1] = midionc(stack[sp - 1], stack[sp]);
				break;
			case FUNC_SCALE:
				sp--;
				stack[sp - 1] = scale(stack[sp - 1], stack[sp]);
				break;
			case FUNC_DISTORT:
				stack[sp - 1] = distort(stack[sp - 1]);
				break;
			case FUNC_CATMULLROM:
				sp -= 4;
				stack[sp - 1] = catmullrom(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3]);
				break;
			case FUNC_NOP:
				sp++;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP1:
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP2:
				sp--;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP3:
				sp-=2;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP4:
				sp -= 3;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP5:
				sp -= 4;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP6:
				sp -= 5;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP7:
				sp -= 6;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP8:
				sp -= 7;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP9:
				sp -= 8;
				stack[sp - 1] = 0;
				break;
			case FUNC_NOP10:
				sp -= 9;
				stack[sp - 1] = 0;
				break;
			case FUNC_IMG:
				sp -= 2;
				stack[sp - 1] = img(stringstore + (int)stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_BARA:
				sp--;
				stack[sp - 1] = bara(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_PIEA:
				sp--;
				stack[sp - 1] = piea(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_LOADWAV:
				stack[sp - 1] = loadwav(stringstore + (int)stack[sp - 1], cell, memofs);
				break;
			case FUNC_LOADWAVC:
				sp--;
				stack[sp - 1] = loadwavc(stringstore + (int)stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_LEN:
				stack[sp - 1] = len(stack[sp - 1]);
				break;
			case FUNC_CHANNELS:
				stack[sp - 1] = channels(stack[sp - 1]);
				break;
			case FUNC_PLAY:
				sp -= 1;
				stack[sp - 1] = play(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_PLAYLOOP:
				sp -= 2;
				stack[sp - 1] = playloop(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_PLAYLOOPP:
				sp -= 4;
				stack[sp - 1] = playloopp(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;
			case FUNC_SAMPLE:
				sp--;
				stack[sp - 1] = sample(stack[sp - 1], stack[sp]);
				break;
			case FUNC_SAMPLEFAST:
				sp--;
				stack[sp - 1] = samplefast(stack[sp - 1], stack[sp]);
				break;
			case FUNC_PLAYLOOPX:
				sp -= 4;
				stack[sp - 1] = playloopx(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;
			case FUNC_GRAIN:
				sp -= 3;
				stack[sp - 1] = grain(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], cell, memofs);
				break;
			case FUNC_BUFFER:
				stack[sp - 1] = buffer(stack[sp - 1], cell, memofs);
				break;
			case FUNC_KLATT:
				stack[sp - 1] = klatt(stringstore + (int)stack[sp - 1], cell, memofs);
				break;
			case FUNC_PADSYNTH:
				sp -= 10;
				stack[sp - 1] = padsynth(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], stack[sp + 4], stack[sp + 5], stack[sp + 6], stack[sp + 7], stack[sp + 8], stack[sp + 9], cell, memofs);
				break;
			case FUNC_PADSYNTH22:
				sp -= 10;
				stack[sp - 1] = padsynth22(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], stack[sp + 4], stack[sp + 5], stack[sp + 6], stack[sp + 7], stack[sp + 8], stack[sp + 9], cell, memofs);
				break;
			case FUNC_LABEL:
				stack[sp - 1] = label(stringstore + (int)stack[sp - 1], cell, memofs);
				break;
			case FUNC_BIQUAD:
				sp -= 6;
				stack[sp - 1] = biquad(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], stack[sp + 4], stack[sp + 5], cell, memofs);
				break;
			case FUNC_WRITE:
				sp -= 3;
				stack[sp - 1] = write(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2]);
				break;
			case FUNC_MIDIOUTC:
				sp-=2;
				stack[sp - 1] = midioutc(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_MIDIOUTPITCHC:
				sp--;
				stack[sp - 1] = midioutpitchc(stack[sp - 1], stack[sp]);
				break;
			case FUNC_MIDIOUTPOTC:
				sp-=2;
				stack[sp - 1] = midioutpotc(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_MIDIOUTPROGC:
				sp--;
				stack[sp - 1] = midioutprogc(stack[sp - 1], stack[sp]);
				break;
			case FUNC_ENCODER:
				stack[sp] = encoder(0, cell, memofs);
				sp++;
				break;
			case FUNC_ENCODERV:
				stack[sp - 1] = encoder(stack[sp - 1], cell, memofs);
				break;
			case FUNC_AY:
				sp -= 10;
				stack[sp - 1] = ay(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], stack[sp + 4], stack[sp + 5], stack[sp + 6], stack[sp + 7], stack[sp + 8], stack[sp + 9], cell, memofs);
				break;
			case FUNC_SIDFILTER:
				sp -= 3;
				stack[sp - 1] = sidfilter(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], cell, memofs);
				break;
			case FUNC_SIDVOICE:
				sp -= 2;
				stack[sp - 1] = sidvoice(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_SIDENVELOPE:
				sp -= 4;
				stack[sp - 1] = sidenvelope(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;
			case FUNC_SLEWLIMIT:
				sp -= 1;
				stack[sp - 1] = slewlimit(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_SLEWLIMITUD:
				sp -= 2;
				stack[sp - 1] = slewlimitud(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_SQUARESAWD:
				sp-=2;
				stack[sp - 1] = squaresawd(stack[sp - 1], stack[sp], stack[sp + 1]);
				break;
			case FUNC_AKWF:
				sp--;
				stack[sp - 1] = akwf(stack[sp - 1], stack[sp]);
				break;
			case FUNC_BBD:
				sp -= 1;
				stack[sp - 1] = bbd(stack[sp - 1], stack[sp], cell, memofs);
				break;
			case FUNC_BBDEFF:
				sp -= 2;
				stack[sp - 1] = bbdeff(stack[sp - 1], stack[sp], stack[sp + 1], cell, memofs);
				break;
			case FUNC_HEXWAVE:
				sp -= 4;
				stack[sp - 1] = hexwave(stack[sp - 1], stack[sp], stack[sp + 1], stack[sp + 2], stack[sp + 3], cell, memofs);
				break;

			default:
				return nan("0");
			}
			memofs += gFunc[func].mMemory + gFunc[func].mSamplerateMemory * gSamplerate;
		}
		break;
		case '*':
		{
			sp--;
			stack[sp - 1] *= stack[sp];
			bc++;
		}
		break;
		case '/':
		{
			sp--;
			if (stack[sp] == 0)
			{
				stack[sp - 1] = nan("0");
			}
			else
			{
				stack[sp - 1] /= stack[sp];
			}
			bc++;
		}
		break;
		case '+':
		{
			sp--;
			stack[sp - 1] += stack[sp];
			bc++;
		}
		break;
		case '-':
		{
			sp--;
			stack[sp - 1] -= stack[sp];
			bc++;
		}
		break;
		case '>':
		{
			sp--;
			stack[sp - 1] = (stack[sp - 1] > stack[sp]) ? 1.0 : 0.0;
			bc++;
		}
		break;
		case '<':
		{
			sp--;
			stack[sp - 1] = (stack[sp - 1] < stack[sp]) ? 1.0 : 0.0;
			bc++;
		}
		break;
		case '=':
		{
			sp--;
			stack[sp - 1] = (fabs(stack[sp - 1] - stack[sp]) < 0.0001) ? 1.0 : 0.0;
			bc++;
		}
		break;
		default:
			return nan("0");
		}
	}
	if (sp != 1)
		return nan("0");
	return stack[0];
}
