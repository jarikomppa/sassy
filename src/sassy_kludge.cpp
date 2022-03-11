#include "sassy.h"

int gSC_EnableColoring = 0;
int gSC_Skip = 0;
int gSC_PrevCol = 0;
char* gSC_Brace1 = 0;
char* gSC_Brace2 = 0;

int editor_callback(ImGuiInputTextCallbackData* data)
{
	gSC_Brace1 = 0;
	gSC_Brace2 = 0;
	if (data->Buf[data->CursorPos] == '(')
	{
		int braces = 1;
		int i = data->CursorPos + 1;
		while (data->Buf[i] && braces)
		{
			if (data->Buf[i] == '(') braces++;
			if (data->Buf[i] == ')') braces--;
			i++;
		}
		if (!braces)
		{
			gSC_Brace1 = data->Buf + data->CursorPos;
			gSC_Brace2 = data->Buf + i - 1;
		}
	}
	if (data->Buf[data->CursorPos] == ')')
	{
		int braces = 1;
		int i = data->CursorPos - 1;
		while (i >= 0 && braces)
		{
			if (data->Buf[i] == ')') braces++;
			if (data->Buf[i] == '(') braces--;
			i--;
		}
		if (!braces)
		{
			gSC_Brace1 = data->Buf + data->CursorPos;
			gSC_Brace2 = data->Buf + i + 1;
		}
	}

	if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
	{
		char temp[32];
		int i = data->CursorPos - 1;
		while (i > 0 && isalpha(data->Buf[i]))
			i--;
		if (i > 0)
			i++;
		if (data->CursorPos - i > 31 || data->CursorPos - i < 1)
			return 0;
		memcpy(temp, data->Buf + i, data->CursorPos - i);
		temp[data->CursorPos - i] = 0;
		const char * t = find_tab(temp);
		if (t)
		{
			int low = temp[0] > 'a';
			sprintf(temp, "%s()", t);
			if (low)
			{
				i = 0;
				while (temp[i])
				{
					temp[i] = (char)tolower(temp[i]);
					i++;
				}
			}
			
			data->InsertChars(data->CursorPos, temp);
			data->CursorPos--;
		}
	}
	return 0;
}

ImU32 syntaxColoring(const char* s, const char* text_end, ImU32 origcol)
{
	if (!gSC_EnableColoring)
		return origcol;

	if (gSC_Skip)
	{
		gSC_Skip--;
		return gSC_PrevCol;
	}

	if (gConfig.mBraceColoring && (s == gSC_Brace1 || s == gSC_Brace2))
		return 0xffffffff;

	if (!gConfig.mSyntaxColoring)
		return origcol;

	char temp[32];
	if (isalpha(*s))
	{
		int i = 0;
		while (i < 30 && s + i != text_end && (isalpha(s[i]) || isnum(s[i])))
		{
			temp[i] = s[i];
			i++;
		}
		temp[i] = 0;
		int v = is_symbol(temp);
		if (v < 0)
		{
			gSC_PrevCol = 0xffdcdc7d;
			gSC_Skip = i-1;
			return gSC_PrevCol;
		}
		else
			if (v > 0)
			{
				gSC_PrevCol = 0xff9cdcfe;
				gSC_Skip = i-1;
				return gSC_PrevCol;
			}
			else
			{
				gSC_PrevCol = origcol;
				gSC_Skip = i-1;
				return gSC_PrevCol;
			}		
	}
	if (*s == ':')
		return 0xff9cdcfe; // a1:a5
	if (*s == '(' || *s == ')' || *s == ',' || *s == ';')
	{
		return 0xffb4b4b4;
	}
	if (isoper(*s))
	{
		return 0xffd8a0df;
	}
	if (*s == '"')
	{
		int i = 1;
		while (s + i != text_end && s[i] != '"')
		{
			i++;
		}
		gSC_PrevCol = 0xff4173d6;
		gSC_Skip = i;
		return gSC_PrevCol;
	}
	return 0xffb5cea8; // what remains should be numbers
}
