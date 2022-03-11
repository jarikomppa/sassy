#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

int precedence(Op &v)
{
	if (v.mOpcode == '*' || v.mOpcode == '/') return 4;
	if (v.mOpcode == '+' || v.mOpcode == '-') return 3;
	if (v.mOpcode == '(') return 0;
	return 1;
}

int postfix(Op* bc)
{
	int mem = 0;
	Op temp[1024];
	OpContainer in(temp);
	OpContainer out(bc);
	while (out.peek()->mOpcode)
	{
		*in.next() = *out.next();
	}
	*in.next() = *out.next();
	in.rewind();
	out.rewind();

	Op opstack[2560];
	int sp = 0;

	while (in.peek()->mOpcode)
	{
		if (in.peek()->mOpcode == 'C')
		{
			*out.next() = *in.next();
		}
		if (in.peek()->mOpcode == 'V' || in.peek()->mOpcode == 'A' || in.peek()->mOpcode == 'T')
		{
			*out.next() = *in.next();
		}
		if (in.peek()->mOpcode == 'F')
		{
			mem += gFunc[in.peek()->mArg.i.i].mMemory + gFunc[in.peek()->mArg.i.i].mSamplerateMemory * gSamplerate;
			opstack[sp] = *in.next();
			sp++;
		}
		if (isoper(in.peek()->mOpcode))
		{
			while (sp && isoper(opstack[sp - 1].mOpcode) && precedence(opstack[sp - 1]) >= precedence(*in.peek()))
			{
				sp--;
				*out.next() = opstack[sp];
			}
			opstack[sp] = *in.next(); 
			sp++;
		}
		if (in.peek()->mOpcode == '(')
		{
			opstack[sp] = *in.next(); 
			sp++;
		}
		if (in.peek()->mOpcode == ')')
		{
			sp--; in.next();

			while (opstack[sp].mOpcode != '(')
			{
				*out.next() = opstack[sp]; 
				sp--;
			}
			if (sp && opstack[sp - 1].mOpcode == 'F')
			{
				sp--;
				*out.next() = opstack[sp];
			}
		}
	}
	while (sp)
	{
		sp--;
		*out.next() = opstack[sp];
	}
	out.next()->mOpcode = 0;
	return mem;
}