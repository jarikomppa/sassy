#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "eval.h"

bool isnum(char c)
{
	return (c >= '0' && c <= '9' || c == '.');
}

bool isws(char c)
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

bool isoper(int c)
{
	return (c == '-' || c == '+' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>');
}

char toupper(char c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	return c;
}

bool isalpha(char c)
{
	c = toupper(c);
	return (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

int isvar(const char* s)
{	
	int v = is_symbol(s);
	if (v <= 0) return 0;
	return v;
}

int isarea(const char* s, Op* dst = 0)
{
	char temp[32];
	int i = 0;
	while (i < 31 && s[i] && s[i] != ':')
	{
		temp[i] = s[i];
		i++;
	}
	if (s[i] != ':') return 0;
	temp[i] = 0;
	int a = isvar(temp);
	if (a <= 0) return 0;
	i++;
	int j = 0;
	while (i+j < 31 && s[i])
	{
		temp[j] = s[i];
		i++;
		j++;
	}
	temp[j] = 0;
	int b = isvar(temp);
	if (b <= 0) return 0;
	if (dst)
	{
		dst->mArg.i.i = a;
		dst->mArg.i.j = b;
	}
	return 1;
}

int isfunc(const char* s)
{
	int v = is_symbol(s);
	if (is_symbol(s) < 0) 
		return -v;
	return 0;
}

void parse(const char* s, Op* bc, char* textstore)
{
	//	const char* os = s;
	bool expect_number = true;
	int textoffset = 0;
	OpContainer out(bc);
	out.peek()->mOpcode = 0;
	while (*s)
	{
		while (*s && isws(*s)) s++;
		//printf("[%c]", *s);
		if (!*s)
		{
			// Whitespace at end of code
		}
		else
			if (*s == '"')
			{
				out.peek()->mOpcode = 'T';
				out.next()->mArg.i.i = textoffset;
				s++;
				while (*s && *s != '"')
				{
					textstore[textoffset++] = *s;
					s++; 
				}
				if (*s) s++;
				textstore[textoffset++] = 0;
				expect_number = false;
			}
			else
			if (*s == ',' || *s == ';')
			{
				s++;
				out.next()->mOpcode = ',';
				expect_number = true;
			}
			else
				if (expect_number && (isnum(*s) || *s == '-'))
				{
					float neg = 1;
					if (*s == '-')
					{
						neg = -1;
						s++;
					}
					float postpoint = 0;
					float v = 0;
					while (isnum(*s))
					{
						if (*s == '.')
						{
							postpoint = 10;
						}
						else
						{
							if (!postpoint)
							{
								v *= 10;
								v += *s - '0';
							}
							else
							{
								v += (*s - '0') / postpoint;
								postpoint *= 10;
							}
						}
						s++;
					}
					if (neg && v == 0 && s[-1] == '-')
					{
						// Just a -, no number
						out.peek()->mOpcode = 'C';
						out.next()->mArg.f = -1.0f;
						out.next()->mOpcode = '*';
					}
					else
					{
						out.peek()->mOpcode = 'L';
						out.next()->mArg.f = v * neg;
					}
					out.peek()->mOpcode = 0; // todo: wtf
					expect_number = false;
				}
				else
					if (isoper(*s) || *s == '(' || *s == ')')
					{
						if (isoper(*s) || *s == '(')
							expect_number = true;
						else
							expect_number = false;
						out.next()->mOpcode = *s;
						s++;
					}
					else
						if (isalpha(*s))
						{
							// area, variable or function
							char temp[256];
							char c = 0;
							while (isalpha(*s) || isnum(*s) || *s == ':')
							{
								temp[c] = *s;
								s++; c++;
							}
							temp[c] = 0;
							if (isarea(temp))
							{
								out.peek()->mOpcode = 'A';
								isarea(temp, out.next());
							}
							else
							if (isvar(temp))
							{
								out.peek()->mOpcode = 'V';
								out.next()->mArg.i.i = isvar(temp);
							}
							else
								if (isfunc(temp))
								{
									out.peek()->mOpcode = 'F';
									out.peek()->mArg.i.j = textoffset; // a bit of a kludge to get img widget to work
									out.next()->mArg.i.i = isfunc(temp);
								}
								else
								{
									//printf("Parse error \"%s\" @ \"%s\"\n", os, s);
									bc[0].mOpcode = 'C';
									bc[0].mArg.f = nanf("0");
									bc[1].mOpcode = 0;
									return;
								}
							expect_number = false;
						}
						else
						{
							//printf("Parse error \"%s\" @ \"%s\"\n", os, s);
							bc[0].mOpcode = 'C';
							bc[0].mArg.f = nanf("0");
							bc[1].mOpcode = 0;
							return;
						}
	}
	out.next()->mOpcode = 0;
}
