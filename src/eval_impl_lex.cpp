#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

bool lex_expr(OpContainer& in, OpContainer& out, bool aArea);

bool lex_paren(OpContainer& in, OpContainer& out)
{
	*out.next() = *in.next(); // '('
	if (lex_expr(in, out, false)) return true;
	if (in.peek()->mOpcode != ')') return true;
	*out.next() = *in.next(); // ')'
	return false;
}


bool lex_function(OpContainer& in, OpContainer& out)
{
	Op* f = in.next();
	char sig[32];
	Op* outfunc = out.peek();
	*out.next() = *f;
	if (in.peek()->mOpcode != '(') return true;
	*out.next() = *in.next();
	int params = 0;
	int has_vars = 0;
	while (in.peek()->mOpcode != ')' && in.peek()->mOpcode != 0)
	{
		if (in.peek()->mOpcode == 'A')
		{
			sig[params] = 'A';
		}
		else
		if (in.peek()->mOpcode == 'V')
		{
			sig[params] = 'V';
			has_vars++;
		}
		else
		if (in.peek()->mOpcode == 'T')
		{
			sig[params] = 'T';
		}
		else
		if (in.peek()->mOpcode == 'L')
		{
			sig[params] = 'L';
		}
		else
		{
			sig[params] = 'C';
		}
		params++;
		out.next()->mOpcode = '('; // surround parameters with parens
		if (lex_expr(in, out, in.peek()->mOpcode == 'A')) return true;
		out.next()->mOpcode = ')';
		if (in.peek()->mOpcode == ',')
		{
			in.next(); // skip ','
		}
	}
	sig[params] = 0;
	if (in.peek()->mOpcode != ')') return true;
	*out.next() = *in.next();
	
	int funcsym = outfunc->mArg.i.i;
	outfunc->mArg.i.i = get_function(funcsym, sig);
	if (outfunc->mArg.i.i == 0)
		return true;
	return false;
}

bool lex_expr(OpContainer& in, OpContainer& out, bool aArea)
{
	Op* op = in.peek();
	if (op->mOpcode == 'C') // expr = constval
	{
		*out.next() = *in.next();
	}
	else
		if (op->mOpcode == 'V' || (op->mOpcode == 'A' && aArea) || op->mOpcode == 'L' || op->mOpcode == 'T') // expr = variable, area, literal
		{
			*out.next() = *in.next();
		}
		else
			if (op->mOpcode == '(') // expr = ( expr )
			{
				if (lex_paren(in, out)) return true;
			}
			else
				if (op->mOpcode == 'F') // expr = function
				{
					if (lex_function(in, out)) return true;
				}
				else
				{
					return true;
				}

	if (isoper(in.peek()->mOpcode))
	{
		*out.next() = *in.next();
		return lex_expr(in, out, false);
	}
	return false;
}

bool lexify(Op* bc)
{
	/*
	* expr = expr operand expr
	* expr = constval
	* expr = variable
	* expr = ( expr )
	* expr = function()
	* expr = function(expr)
	* expr = function(expr, expr)
	* expr = function(expr, expr, expr)
	*/

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

	bool res = lex_expr(in, out, false);
	if (in.peek()->mOpcode != 0) res = true;
	out.next()->mOpcode = 0;
	
	// Replace literals with constants
	out.rewind();
	while (out.peek()->mOpcode)
	{
		if (out.peek()->mOpcode == 'L')
			out.peek()->mOpcode = 'C';
		out.next();
	}
	
	return res;
}
