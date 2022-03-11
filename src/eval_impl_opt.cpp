#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

using namespace EvalFunc;

//#define DEBUGPRINT(x) printf(x);
//#define DODEBUGPRINT
#define DEBUGPRINT(x)

#ifdef DODEBUGPRINT
extern void printtokens(Op* bc, char* logstring);
#endif

void opt(Op* bc)
{
	Op bc1[1024];
	Op bc2[1024];
	OpContainer a(bc1);
	OpContainer b(bc2);
	OpContainer input(bc);
	while (input.peek()->mOpcode)
	{
		*a.next() = *input.next();
	}
	*a.next() = *input.next();
	a.index = 0;

	/*
	 * Does constant folding. Has limits.
	 * 2+3+v -> 5+v (sees constant+constant)
	 * 2+v+3 -> 2+v+3 (doesn't see, as it doesn't know how to reorder)
	 * Simple and quite effective, probably not worth it to complicate.
	 */

	bool changed = true;
#ifdef DODEBUGPRINT
	bool didchange = true;
	while (didchange)
	{
		didchange = false;
#else
	while (changed)
	{
#endif
		changed = false;
		while (a.peek()->mOpcode)
		{
			if (a.peek()->mOpcode == '*' && 
				a.op[a.index - 1].mOpcode == 'C' && 
				a.op[a.index + 1].mOpcode == 'C')
			{
				b.op[b.index - 1].mArg.f *= a.op[a.index + 1].mArg.f;
				a.next(); // '*'
				a.next(); // 'C'
				changed = true;
				DEBUGPRINT("collapsed *\n");
			}
			else
				if (a.peek()->mOpcode == '/' && 
					a.op[a.index - 1].mOpcode == 'C' && 
					a.op[a.index + 1].mOpcode == 'C')
				{
					if (a.op[a.index + 1].mArg.f == 0)
						b.op[b.index - 1].mArg.f = nanf("0");
					else
						b.op[b.index - 1].mArg.f /= a.op[a.index + 1].mArg.f;
					a.next(); // '/'
					a.next(); // 'C'
					changed = true;
					DEBUGPRINT("collapsed /\n");
				}
				else
					if (a.peek()->mOpcode == '+' &&
						a.op[a.index - 1].mOpcode == 'C' &&
						a.op[a.index + 1].mOpcode == 'C' &&
						precedence(a.op[a.index + 2]) <= 3 &&
						(a.index < 2 || precedence(a.op[a.index - 2]) <= 3))
					{
						b.op[b.index - 1].mArg.f += a.op[a.index + 1].mArg.f;
						a.next(); // '+'
						a.next(); // 'C'
						changed = true;
						DEBUGPRINT("collapsed +\n");
					}
					else
						if (a.peek()->mOpcode == '-' &&
							a.op[a.index - 1].mOpcode == 'C' &&
							a.op[a.index + 1].mOpcode == 'C' &&
							precedence(a.op[a.index + 2]) <= 3 &&
							(a.index < 2 || precedence(a.op[a.index - 2]) <= 3))
						{
							b.op[b.index - 1].mArg.f -= a.op[a.index + 1].mArg.f;
							a.next(); // '-'
							a.next(); // 'C'
							changed = true;
							DEBUGPRINT("collapsed -\n");
						}
						else
							if (a.peek()->mOpcode == '<' &&
								a.op[a.index - 1].mOpcode == 'C' &&
								a.op[a.index + 1].mOpcode == 'C' &&
								precedence(a.op[a.index + 2]) <= 3 &&
								(a.index < 2 || precedence(a.op[a.index - 2]) <= 3))
							{
								b.op[b.index - 1].mArg.f = (b.op[b.index - 1].mArg.f < a.op[a.index + 1].mArg.f) ? 1.0 : 0.0;
								a.next(); // '<'
								a.next(); // 'C'
								changed = true;
								DEBUGPRINT("collapsed <\n");
							}
							else
								if (a.peek()->mOpcode == '>' &&
									a.op[a.index - 1].mOpcode == 'C' &&
									a.op[a.index + 1].mOpcode == 'C' &&
									precedence(a.op[a.index + 2]) <= 3 &&
									(a.index < 2 || precedence(a.op[a.index - 2]) <= 2))
								{
									b.op[b.index - 1].mArg.f = (b.op[b.index - 1].mArg.f > a.op[a.index + 1].mArg.f) ? 1.0 : 0.0;
									a.next(); // '>'
									a.next(); // 'C'
									changed = true;
									DEBUGPRINT("collapsed >\n");
								}
								else
										if (a.peek()->mOpcode == '=' &&
											a.op[a.index - 1].mOpcode == 'C' &&
											a.op[a.index + 1].mOpcode == 'C' &&
											precedence(a.op[a.index + 2]) <= 3 &&
											(a.index < 2 || precedence(a.op[a.index - 2]) <= 2))
										{
											b.op[b.index - 1].mArg.f = (fabs(b.op[b.index - 1].mArg.f - a.op[a.index + 1].mArg.f) < 0.0001f) ? 1.0 : 0.0;
											a.next(); // '='
											a.next(); // 'C'
											changed = true;
											DEBUGPRINT("collapsed =\n");
										}
										else
											if (a.peek()->mOpcode == '(' &&
												(a.op[a.index + 1].mOpcode == 'C' || a.op[a.index + 1].mOpcode == 'V' || a.op[a.index + 1].mOpcode == 'A' || a.op[a.index + 1].mOpcode == 'T') &&
												a.op[a.index + 2].mOpcode == ')' &&
												(a.index < 1 || a.op[a.index - 1].mOpcode != 'F'))
											{
												a.next(); // '('
												*b.next() = *a.next(); // 'C', 'V' or 'A'
												a.next(); // ')'
												changed = true;
												DEBUGPRINT("collapsed ()\n");
											}
											else
												// 0 parameter functions
												if (a.peek()->mOpcode == 'F' &&
													gFunc[a.peek()->mArg.i.i].mConst &&
													a.op[a.index + 1].mOpcode == '(' &&
													a.op[a.index + 2].mOpcode == ')')
												{
													b.peek()->mOpcode = 'C';
													switch (a.peek()->mArg.i.i)
													{
													case FUNC_PI:
														b.next()->mArg.f = 3.1415926535897932384626433832795f;
														break;
													case FUNC_NAN:
														b.next()->mArg.f = nan("0");
														break;
													case FUNC_TRUE:
														b.next()->mArg.f = 1.0;
														break;
													case FUNC_FALSE:
														b.next()->mArg.f = 0.0;
														break;
													case FUNC_DT:
														b.next()->mArg.f = dt();
														break;
													case FUNC_NOP:
														b.next()->mArg.f = 0;
														break;
													default:
														b.next()->mArg.f = nan("0");
													}
													a.next(); // 'F'
													a.next(); // '('
													a.next(); // ')'
													changed = true;
													DEBUGPRINT("collapsed F()\n");
												}
												else
													// 1 parameter var functions
													if (a.peek()->mOpcode == 'F' &&
														gFunc[a.peek()->mArg.i.i].mConst &&
														a.op[a.index + 1].mOpcode == '(' &&
														a.op[a.index + 2].mOpcode == 'V' &&
														a.op[a.index + 3].mOpcode == ')')
													{
														// If there's no match, this should be a nop
														switch (a.peek()->mArg.i.i)
														{
														case FUNC_COLUMNOF:
															b.peek()->mOpcode = 'C';
															b.next()->mArg.f = (double)(((a.op[a.index + 2].mArg.i.i) % 27));
															a.next(); // 'F'
															a.next(); // '('
															a.next(); // 'V'
															a.next(); // ')'
															changed = true;
															DEBUGPRINT("collapsed F(V)\n");
															break;
														case FUNC_ROWOF:
															b.peek()->mOpcode = 'C';
															b.next()->mArg.f =(double)(((a.op[a.index + 2].mArg.i.i) / 27) + 1);
															a.next(); // 'F'
															a.next(); // '('
															a.next(); // 'V'
															a.next(); // ')'
															changed = true;
															DEBUGPRINT("collapsed F(V)\n");
															break;
														default:
															*b.next() = *a.next();
														}
													}
												else
														// 1 parameter functions
														if (a.peek()->mOpcode == 'F' &&
															gFunc[a.peek()->mArg.i.i].mConst &&
															a.op[a.index + 1].mOpcode == '(' &&
															a.op[a.index + 2].mOpcode == 'T' &&
															a.op[a.index + 3].mOpcode == ')')
														{
															b.peek()->mOpcode = 'C';
															switch (a.peek()->mArg.i.i)
															{
															case FUNC_REM:
																b.next()->mArg.f = 0;
																break;
															default:
																b.next()->mArg.f = nan("0");
															}
															a.next(); // 'F'
															a.next(); // '('
															a.next(); // 'T'
															a.next(); // ')'
															DEBUGPRINT("collapsed F(T)\n");
															changed = true;
														}
														else
															// 1 parameter functions
													if (a.peek()->mOpcode == 'F' &&
														gFunc[a.peek()->mArg.i.i].mConst &&
														a.op[a.index + 1].mOpcode == '(' &&
														a.op[a.index + 2].mOpcode == 'C' &&
														a.op[a.index + 3].mOpcode == ')')
													{
														b.peek()->mOpcode = 'C';
														switch (a.peek()->mArg.i.i)
														{
														case FUNC_SIN:
															b.next()->mArg.f = sin(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_TRUNC:
															b.next()->mArg.f = trunc(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_FRACT:
															b.next()->mArg.f = a.op[a.index + 2].mArg.f - trunc(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SIN1:
															b.next()->mArg.f = sin1(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_TRIANGLE:
															b.next()->mArg.f = triangle(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SQUARE:
															b.next()->mArg.f = square(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SQUAREQ:
															b.next()->mArg.f = squareq(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SAW:
															b.next()->mArg.f = saw(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SAWQ:
															b.next()->mArg.f = sawq(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_QUANTIZE:
															{
																// pow(2, 3/4.0) ~ 1.6817928305074290860622509524664297900800685247135690216264521719
																double t = 12 * log(32 * POW_2_3_4TH * (a.op[a.index + 2].mArg.f) / 440) / log(2);
																if (t >= 0 && t < 128)
																	t = note_to_freq[(int)t];
																b.next()->mArg.f = t;
															}
															break;
														case FUNC_SQRT:
															b.next()->mArg.f = sqrt(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_LOG:
															b.next()->mArg.f = log(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_COS:
															b.next()->mArg.f = cos(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_TAN:
															b.next()->mArg.f = tan(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_ACOS:
															b.next()->mArg.f = acos(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_ASIN:
															b.next()->mArg.f = asin(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_ATAN:
															b.next()->mArg.f = atan(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_COSH:
															b.next()->mArg.f = cosh(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SINH:
															b.next()->mArg.f = sinh(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_TANH:
															b.next()->mArg.f = tanh(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_EXP:
															b.next()->mArg.f = exp(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_LOG10:
															b.next()->mArg.f = log10(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_FLOOR:
															b.next()->mArg.f = floor(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_ABS:
															b.next()->mArg.f = fabs(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_NOTETOFREQ:
															if (a.op[a.index + 2].mArg.f >= 0 && a.op[a.index + 2].mArg.f < 128)
																b.next()->mArg.f = note_to_freq[(int)a.op[a.index + 2].mArg.f];
															else
																b.next()->mArg.f = a.op[a.index + 2].mArg.f;
															break;
														case FUNC_FREQTONOTE:
															b.next()->mArg.f = 12 * log(32 * POW_2_3_4TH * (a.op[a.index + 2].mArg.f / 440)) / log(2);
															break;
														case FUNC_ISNAN:
															b.next()->mArg.f = isnormal(a.op[a.index + 2].mArg.f) ? 0 : 1;
															break;
														case FUNC_NANKILL:
															b.next()->mArg.f = !isnormal(a.op[a.index + 2].mArg.f) ? 0 : a.op[a.index + 2].mArg.f;
															break;
														case FUNC_NOT:
															b.next()->mArg.f = (!(a.op[a.index + 2].mArg.f > 0.0001)) ? 1.0 : 0.0;
															break;
														case FUNC_DEGREES:
															b.next()->mArg.f = a.op[a.index + 2].mArg.f * (180 / M_PI);
															break;
														case FUNC_RADIANS:
															b.next()->mArg.f = a.op[a.index + 2].mArg.f * (M_PI / 180);
															break;
														case FUNC_EVEN:
															b.next()->mArg.f = ((int)a.op[a.index + 2].mArg.f & 1) ? 0.0 : 1.0;
															break;
														case FUNC_ODD:
															b.next()->mArg.f = ((int)a.op[a.index + 2].mArg.f & 1) ? 1.0 : 0.0;
															break;
														case FUNC_SIGN:
															b.next()->mArg.f = (a.op[a.index + 2].mArg.f < 0.0) ? -1.0 : 1.0;
															break;
														case FUNC_SMOOTHSTEP:
															b.next()->mArg.f = a.op[a.index + 2].mArg.f * a.op[a.index + 2].mArg.f * (3 - 2 * a.op[a.index + 2].mArg.f);
															break;
														case FUNC_SMOOTHERSTEP:
															b.next()->mArg.f = a.op[a.index + 2].mArg.f * a.op[a.index + 2].mArg.f * a.op[a.index + 2].mArg.f * (a.op[a.index + 2].mArg.f * (a.op[a.index + 2].mArg.f * 6 - 15) + 10);
															break;
														case FUNC_OPL1:
															b.next()->mArg.f = opl1(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_OPL2:
															b.next()->mArg.f = opl1(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_OPL3:
															b.next()->mArg.f = opl1(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_NOTETOFREQSLOW:
															b.next()->mArg.f = pow(2, (a.op[a.index + 2].mArg.f - 69) / 12.0) * 440;
															break;
														case FUNC_B_NOT:
															b.next()->mArg.f = b_not(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_DISTORT:
															b.next()->mArg.f = distort(a.op[a.index + 2].mArg.f);
															break;
														case FUNC_NOP1:
															b.next()->mArg.f = 0;
															break;
														default:
															b.next()->mArg.f = nan("0");
														}
														a.next(); // 'F'
														a.next(); // '('
														a.next(); // 'C'
														a.next(); // ')'
														DEBUGPRINT("collapsed F(C)\n");
														changed = true;
													}
													else
														// 2 parameter functions
														if (a.peek()->mOpcode == 'F' &&
															gFunc[a.peek()->mArg.i.i].mConst &&
															a.op[a.index + 1].mOpcode == '(' &&
															a.op[a.index + 2].mOpcode == 'C' &&
															a.op[a.index + 3].mOpcode == 'C' &&
															a.op[a.index + 4].mOpcode == ')')
														{
															b.peek()->mOpcode = 'C';
															switch (a.peek()->mArg.i.i)
															{
															case FUNC_MOD:
																if (a.op[a.index + 3].mArg.f == 0)
																	b.next()->mArg.f = 0;
																else
																	b.next()->mArg.f = fmod(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_PULSE:
																b.next()->mArg.f = pulse(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_POW:
																b.next()->mArg.f = pow(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_ATAN2:
																b.next()->mArg.f = atan2(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_SQUAREF:
																b.next()->mArg.f = squaref(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_SAWF:
																b.next()->mArg.f = sawf(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_SQUARESAW:
																b.next()->mArg.f = squaresaw(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_AND:
																b.next()->mArg.f = ((a.op[a.index + 2].mArg.f > 0.0001) && (a.op[a.index + 3].mArg.f > 0.0001)) ? 1.0 : 0.0;
																break;
															case FUNC_OR:
																b.next()->mArg.f = ((a.op[a.index + 2].mArg.f > 0.0001) || (a.op[a.index + 3].mArg.f > 0.0001)) ? 1.0 : 0.0;
																break;
															case FUNC_XOR:
																b.next()->mArg.f = ((a.op[a.index + 2].mArg.f > 0.0001) ^ (a.op[a.index + 3].mArg.f > 0.0001)) ? 1.0 : 0.0;
																break;
															case FUNC_MIN:
																b.next()->mArg.f = (a.op[a.index + 2].mArg.f > a.op[a.index + 3].mArg.f) ? a.op[a.index + 3].mArg.f : a.op[a.index + 2].mArg.f;
																break;
															case FUNC_MAX:
																b.next()->mArg.f = (a.op[a.index + 2].mArg.f < a.op[a.index + 3].mArg.f) ? a.op[a.index + 3].mArg.f : a.op[a.index + 2].mArg.f;
																break;
															case FUNC_B_AND:
																b.next()->mArg.f = b_and(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_OR:
																b.next()->mArg.f = b_or(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_XOR:
																b.next()->mArg.f = b_xor(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_NAND:
																b.next()->mArg.f = b_nand(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_NOR:
																b.next()->mArg.f = b_nor(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_SHLEFT:
																b.next()->mArg.f = b_shleft(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_SHRIGHT:
																b.next()->mArg.f = b_shright(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_ROTLEFT:
																b.next()->mArg.f = b_rotleft(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, 32);
																break;
															case FUNC_B_ROTRIGHT:
																b.next()->mArg.f = b_rotright(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, 32);
																break;
															case FUNC_B_TEST:
																b.next()->mArg.f = b_test(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_SET:
																b.next()->mArg.f = b_set(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_B_CLEAR:
																b.next()->mArg.f = b_clear(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_BITCRUNCH:
																b.next()->mArg.f = bitcrunch(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_SCALE:
																b.next()->mArg.f = scale(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f);
																break;
															case FUNC_NOP2:
																b.next()->mArg.f = 0;
																break;
															default:
																b.next()->mArg.f = nanf("0");
															}
															a.next(); // 'F'
															a.next(); // '('
															a.next(); // 'C'
															a.next(); // 'C'
															a.next(); // ')'
															changed = true;
															DEBUGPRINT("collapsed F(CC)\n");
														}
														else
															// 3 parameter functions
															if (a.peek()->mOpcode == 'F' &&
																gFunc[a.peek()->mArg.i.i].mConst &&
																a.op[a.index + 1].mOpcode == '(' &&
																a.op[a.index + 2].mOpcode == 'C' &&
																a.op[a.index + 3].mOpcode == 'C' &&
																a.op[a.index + 4].mOpcode == 'C' &&
																a.op[a.index + 5].mOpcode == ')')
															{
																b.peek()->mOpcode = 'C';
																switch (a.peek()->mArg.i.i)
																{
																case FUNC_IF:
																	b.next()->mArg.f = a.op[a.index + 2].mArg.f != 0 ? a.op[a.index + 3].mArg.f : a.op[a.index + 4].mArg.f;
																	break;
																case FUNC_SUPERSAW:
																	b.next()->mArg.f = supersaw(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_SUPERSQUARE:
																	b.next()->mArg.f = supersquare(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_SUPERSIN:
																	b.next()->mArg.f = supersin(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_MIX:
																	b.next()->mArg.f = a.op[a.index + 3].mArg.f + a.op[a.index + 2].mArg.f * (a.op[a.index + 4].mArg.f - a.op[a.index + 3].mArg.f);
																	break;
																case FUNC_CLAMP:
																	b.next()->mArg.f = (a.op[a.index + 2].mArg.f < a.op[a.index + 3].mArg.f) ? a.op[a.index + 3].mArg.f : (a.op[a.index + 2].mArg.f > a.op[a.index + 4].mArg.f) ? a.op[a.index + 4].mArg.f : a.op[a.index + 2].mArg.f;
																	break;
																case FUNC_B_ROTLEFTV:
																	b.next()->mArg.f = b_rotleft(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_B_ROTRIGHTV:
																	b.next()->mArg.f = b_rotright(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_RGB:
																	b.next()->mArg.f = rgb(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;
																case FUNC_NOP3:
																	b.next()->mArg.f = 0;
																	break;
																case FUNC_SQUARESAWD:
																	b.next()->mArg.f = squaresawd(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f);
																	break;

																default:
																	b.next()->mArg.f = nanf("0");
																}
																a.next(); // 'F'
																a.next(); // '('
																a.next(); // 'C'
																a.next(); // 'C'
																a.next(); // 'C'
																a.next(); // ')'
																changed = true;
																DEBUGPRINT("collapsed F(CCC)\n");
															}
															else
																// 4 parameter functions
																if (a.peek()->mOpcode == 'F' &&
																	gFunc[a.peek()->mArg.i.i].mConst &&
																	a.op[a.index + 1].mOpcode == '(' &&
																	a.op[a.index + 2].mOpcode == 'C' &&
																	a.op[a.index + 3].mOpcode == 'C' &&
																	a.op[a.index + 4].mOpcode == 'C' &&
																	a.op[a.index + 5].mOpcode == 'C' &&
																	a.op[a.index + 6].mOpcode == ')')
																{
																	b.peek()->mOpcode = 'C';
																	switch (a.peek()->mArg.i.i)
																	{
																	case FUNC_NOP4:
																		b.next()->mArg.f = 0;
																		break;

																	default:
																		b.next()->mArg.f = nanf("0");
																	}
																	a.next(); // 'F'
																	a.next(); // '('
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // ')'
																	changed = true;
																	DEBUGPRINT("collapsed F(CCCC)\n");
																}
															else
																// 5 parameter functions
																if (a.peek()->mOpcode == 'F' &&
																	gFunc[a.peek()->mArg.i.i].mConst &&
																	a.op[a.index + 1].mOpcode == '(' &&
																	a.op[a.index + 2].mOpcode == 'C' &&
																	a.op[a.index + 3].mOpcode == 'C' &&
																	a.op[a.index + 4].mOpcode == 'C' &&
																	a.op[a.index + 5].mOpcode == 'C' &&
																	a.op[a.index + 6].mOpcode == 'C' &&
																	a.op[a.index + 7].mOpcode == ')')
																{
																	b.peek()->mOpcode = 'C';
																	switch (a.peek()->mArg.i.i)
																	{
																	case FUNC_MAP:
																		b.next()->mArg.f = ((a.op[a.index + 2].mArg.f - a.op[a.index + 3].mArg.f) / (a.op[a.index + 4].mArg.f - a.op[a.index + 3].mArg.f)) * (a.op[a.index + 6].mArg.f - a.op[a.index + 5].mArg.f) + a.op[a.index + 5].mArg.f;
																		break;
																	case FUNC_CATMULLROM:
																		b.next()->mArg.f = catmullrom(a.op[a.index + 2].mArg.f, a.op[a.index + 3].mArg.f, a.op[a.index + 4].mArg.f, a.op[a.index + 5].mArg.f, a.op[a.index + 6].mArg.f);
																		break;
																	case FUNC_NOP5:
																		b.next()->mArg.f = 0;
																		break;

																	default:
																		b.next()->mArg.f = nanf("0");
																	}
																	a.next(); // 'F'
																	a.next(); // '('
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // 'C'
																	a.next(); // ')'
																	changed = true;
																	DEBUGPRINT("collapsed F(CCCCC)\n");
																}
																else
																	// 6 parameter functions
																	if (a.peek()->mOpcode == 'F' &&
																		gFunc[a.peek()->mArg.i.i].mConst &&
																		a.op[a.index + 1].mOpcode == '(' &&
																		a.op[a.index + 2].mOpcode == 'C' &&
																		a.op[a.index + 3].mOpcode == 'C' &&
																		a.op[a.index + 4].mOpcode == 'C' &&
																		a.op[a.index + 5].mOpcode == 'C' &&
																		a.op[a.index + 6].mOpcode == 'C' &&
																		a.op[a.index + 7].mOpcode == 'C' &&
																		a.op[a.index + 8].mOpcode == ')')
																	{
																		b.peek()->mOpcode = 'C';
																		switch (a.peek()->mArg.i.i)
																		{
																		case FUNC_NOP6:
																			b.next()->mArg.f = 0;
																			break;

																		default:
																			b.next()->mArg.f = nanf("0");
																		}
																		a.next(); // 'F'
																		a.next(); // '('
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // ')'
																		changed = true;
																		DEBUGPRINT("collapsed F(CCCCCC)\n");
																	}
																	else
																		// 7 parameter functions
																		if (a.peek()->mOpcode == 'F' &&
																			gFunc[a.peek()->mArg.i.i].mConst &&
																			a.op[a.index + 1].mOpcode == '(' &&
																			a.op[a.index + 2].mOpcode == 'C' &&
																			a.op[a.index + 3].mOpcode == 'C' &&
																			a.op[a.index + 4].mOpcode == 'C' &&
																			a.op[a.index + 5].mOpcode == 'C' &&
																			a.op[a.index + 6].mOpcode == 'C' &&
																			a.op[a.index + 7].mOpcode == 'C' &&
																			a.op[a.index + 8].mOpcode == 'C' &&
																			a.op[a.index + 9].mOpcode == ')')
																		{
																			b.peek()->mOpcode = 'C';
																			switch (a.peek()->mArg.i.i)
																			{
																			case FUNC_NOP7:
																				b.next()->mArg.f = 0;
																				break;

																			default:
																				b.next()->mArg.f = nanf("0");
																			}
																			a.next(); // 'F'
																			a.next(); // '('
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // 'C'
																			a.next(); // ')'
																			changed = true;
																			DEBUGPRINT("collapsed F(CCCCCCC)\n");
																		}
																		else
																			// 8 parameter functions
																			if (a.peek()->mOpcode == 'F' &&
																				gFunc[a.peek()->mArg.i.i].mConst &&
																				a.op[a.index + 1].mOpcode == '(' &&
																				a.op[a.index + 2].mOpcode == 'C' &&
																				a.op[a.index + 3].mOpcode == 'C' &&
																				a.op[a.index + 4].mOpcode == 'C' &&
																				a.op[a.index + 5].mOpcode == 'C' &&
																				a.op[a.index + 6].mOpcode == 'C' &&
																				a.op[a.index + 7].mOpcode == 'C' &&
																				a.op[a.index + 8].mOpcode == 'C' &&
																				a.op[a.index + 9].mOpcode == 'C' &&
																				a.op[a.index + 10].mOpcode == ')')
																			{
																				b.peek()->mOpcode = 'C';
																				switch (a.peek()->mArg.i.i)
																				{
																				case FUNC_NOP8:
																					b.next()->mArg.f = 0;
																					break;

																				default:
																					b.next()->mArg.f = nanf("0");
																				}
																				a.next(); // 'F'
																				a.next(); // '('
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // 'C'
																				a.next(); // ')'
																				changed = true;
																				DEBUGPRINT("collapsed F(CCCCCCCC)\n");
																			}
																			else
																				// 9 parameter functions
																				if (a.peek()->mOpcode == 'F' &&
																					gFunc[a.peek()->mArg.i.i].mConst &&
																					a.op[a.index + 1].mOpcode == '(' &&
																					a.op[a.index + 2].mOpcode == 'C' &&
																					a.op[a.index + 3].mOpcode == 'C' &&
																					a.op[a.index + 4].mOpcode == 'C' &&
																					a.op[a.index + 5].mOpcode == 'C' &&
																					a.op[a.index + 6].mOpcode == 'C' &&
																					a.op[a.index + 7].mOpcode == 'C' &&
																					a.op[a.index + 8].mOpcode == 'C' &&
																					a.op[a.index + 9].mOpcode == 'C' &&
																					a.op[a.index + 10].mOpcode == 'C' &&
																					a.op[a.index + 11].mOpcode == ')')
																				{
																					b.peek()->mOpcode = 'C';
																					switch (a.peek()->mArg.i.i)
																					{
																					case FUNC_NOP9:
																						b.next()->mArg.f = 0;
																						break;

																					default:
																						b.next()->mArg.f = nanf("0");
																					}
																					a.next(); // 'F'
																					a.next(); // '('
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // 'C'
																					a.next(); // ')'
																					changed = true;
																					DEBUGPRINT("collapsed F(CCCCCCCCC)\n");
																				}
																				else
																	// 10 parameter functions
																	if (a.peek()->mOpcode == 'F' &&
																		gFunc[a.peek()->mArg.i.i].mConst &&
																		a.op[a.index + 1].mOpcode == '(' &&
																		a.op[a.index + 2].mOpcode == 'C' &&
																		a.op[a.index + 3].mOpcode == 'C' &&
																		a.op[a.index + 4].mOpcode == 'C' &&
																		a.op[a.index + 5].mOpcode == 'C' &&
																		a.op[a.index + 6].mOpcode == 'C' &&
																		a.op[a.index + 7].mOpcode == 'C' &&
																		a.op[a.index + 8].mOpcode == 'C' &&
																		a.op[a.index + 9].mOpcode == 'C' &&
																		a.op[a.index + 10].mOpcode == 'C' &&
																		a.op[a.index + 11].mOpcode == 'C' &&
																		a.op[a.index + 12].mOpcode == ')')
																	{
																		b.peek()->mOpcode = 'C';
																		switch (a.peek()->mArg.i.i)
																		{
																		case FUNC_NOP10:
																			b.next()->mArg.f = 0;
																			break;

																		default:
																			b.next()->mArg.f = nanf("0");
																		}
																		a.next(); // 'F'
																		a.next(); // '('
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // 'C'
																		a.next(); // ')'
																		changed = true;
																		DEBUGPRINT("collapsed F(CCCCCCCCCC)\n");
																	}
																	else
																	{
																		*b.next() = *a.next();
																	}
		

#ifdef DODEBUGPRINT
			if (changed)
			{
				while (a.peek()->mOpcode)
				{
					*b.next() = *a.next();
				}
				*b.peek() = *a.peek();
				char temp[2048];
				temp[0] = 0;
				printtokens(b.op, temp);
				printf("%s\n", temp);
				didchange = true;
				changed = false;
			}
#endif
		}
		*b.next() = *a.next(); // 0
		Op* t = a.op;
		a.op = b.op;
		b.op = t;
		a.index = 0;
		b.index = 0;
	}
	input.index = 0;
	while (a.peek()->mOpcode)
	{
		*input.next() = *a.next();
	}
	*input.next() = *a.next();
}
