#include "sassy.h"

void replace_encoding(int cell, char enc)
{
    int was_text = gCelldata[cell].mRawtext[0] == '\'';
    if (is_value_prefix(gCelldata[cell].mRawtext[0]) || gCelldata[cell].mRawtext[0] == '\'')
    {
        gCelldata[cell].mRawtext[0] = enc;
    }
    else
    {
        // no existing encoding, have to shift the data.
        memmove(gCelldata[cell].mRawtext + 1, gCelldata[cell].mRawtext, 1023); // could move less, but would need to know how much
        gCelldata[cell].mRawtext[0] = enc;
    }
    int is_text = gCelldata[cell].mRawtext[0] == '\'';
    if (was_text != is_text)
        poke(cell);
}

void replace_encoding_area(int x0, int y0, int x1, int y1, char enc)
{
    for (int i = y0; i <= y1; i++)
        for (int j = x0; j <= x1; j++)
            replace_encoding(i * 27 + j, enc);
}


void bake(int cell, int mode)
{
    char temp[1025];
    memcpy(temp, gCelldata[cell].mRawtext, 1025);
    char* s = temp;
    char* d = gCelldata[cell].mRawtext;
    while (*s)
    {
        if ((mode == 0 || mode == 1) && match("slider", s) ||
            (mode == 0 || mode == 2) && match("toggle", s))
        {
            while (*s != '(') s++;
            s++;
            if (*s == ')')
            {
                // empty!
                *d = '0';
                d++;
            }
            while (*s != ')')
            {
                *d = *s;
                d++; s++;
            }
            s++;
        }
        if ((mode == 0 || mode == 3) && match("midipot", s))
        {
            while (*s != '(') s++;
            s++;
            // check if this is unmoved..
            char* t = s;
            int braces = 0;
            while (*t != 0 && (braces || (*t != ',' && *t != ')')))
            {
                if (*t == '(') braces++;
                if (*t == ')') braces--;
                t++;
            }
            if (*t == ')')
            {
                // Empty!
                *d = '0';
                d++;
            }
            else
            {
                while (*s != ',')
                {
                    *d = *s;
                    d++; s++;
                }
                s++;
            }
            braces = 0;
            while (braces || *s != ')')
            {
                if (*s == '(') braces++;
                if (*s == ')') braces--;
                s++;
            }
            s++;
        }
        *d = *s;
        d++;
        s++;
    }
    *d = 0;
    poke(cell);
}

void bake_area(int x0, int y0, int x1, int y1, int mode)
{
    for (int i = y0; i <= y1; i++)
        for (int j = x0; j <= x1; j++)
            bake(i * 27 + j, mode);
}

void setcellvalue_area(int x0, int y0, int x1, int y1, double v)
{
    for (int i = y0; i <= y1; i++)
        for (int j = x0; j <= x1; j++)
            setcellvalue(i * 27 + j, v);
}

void delrowcol(int row)
{
    int x0, y0, x1, y1;
    int a1 = gArea1;
    int a2 = gArea2;
    if (a1 == -1) a1 = a2 = gEditorcell;
    cell_to_xy(a1, x0, y0);
    cell_to_xy(a2, x1, y1);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
    
    if (row)
    {
        x0 = 0;
        x1 = 26;
    }
    else
    {
        y0 = 0;
        y1 = MAXROWS-1;
    }
    a1 = x0 + y0 * 27;
    a2 = x1 + y1 * 27;

    do_del_area(a1, a2);

    int origcopyofs = gCopyofs;

    int b1, b2;

    if (row)
    {
        b1 = x0 + (y1 + 1) * 27;
        b2 = x1 + (MAXROWS - 1) * 27;
    }
    else
    {
        b1 = (x1 + 1) + y0 * 27;
        b2 = 26 + y1 * 27;
    }

    std::string data = do_copy_area(b1, b2);
    do_del_area(b1, b2);
    a2 = a1;
    do_paste_area(a1, a2, data.c_str());
    gCopyofs = origcopyofs;
}

void addrowcol(int num, int lrud, int row)
{
    int x0, y0, x1, y1;
    int a1, a2;
    a1 = a2 = gEditorcell;
    cell_to_xy(a1, x0, y0);
    cell_to_xy(a2, x1, y1);
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    if (row)
    {
        x0 = 0;
        x1 = 26;
        y0 += lrud;
        y1 = MAXROWS - 1 - num;
    }
    else
    {
        y0 = 0;
        x0 += lrud;
        y1 = MAXROWS - 1;
        x1 = 26 - num;
    }
    a1 = x0 + y0 * 27;
    a2 = x1 + y1 * 27;

    int origcopyofs = gCopyofs;

    int b1, b2;

    if (row)
    {
        b2 = b1 = x0 + (y0 + num) * 27;;
    }
    else
    {
        b2 = b1 = x0 + num + y0 * 27;;
    }

    std::string data = do_copy_area(a1, a2);
    do_del_area(a1, a2);
    
    do_paste_area(b1, b2, data.c_str());
    gCopyofs = origcopyofs;
}

void context_menu(int cell, int aArea1, int aArea2)
{   
    
    if (ImGui::BeginPopupContextItem(NULL)) // use id from previous item
    {
        char temp[256];
        int x0, y0, x1, y1, x, y;
        cell_to_xy(cell, x, y);
        cell_to_xy(aArea1, x0, y0);
        cell_to_xy(aArea2, x1, y1);
        if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
        if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

        char cellname[16];

        if (aArea1 == -1 || aArea1 == aArea2)
        {
            sprintf(cellname, "%c%d", 'A' + x, y + 1);
            ImGui::Text("Cell %s", cellname);
            x0 = x1 = x;
            y0 = y1 = y;
        }
        else
        {
            sprintf(cellname, "%c%d:%c%d", 'A' + x0, y0 + 1, 'A' + x1, y1 + 1);
            ImGui::Text("Area %s", cellname);
        }
        sprintf(temp, "Copy \"%s\"", cellname);
        if (ImGui::MenuItem(temp))
        {
            ImGui::SetClipboardText(cellname);
        }

        if (ImGui::BeginMenu("Format as.."))
        {
            if (ImGui::MenuItem("Value"))
                replace_encoding_area(x0, y0, x1, y1, '=');
            if (ImGui::MenuItem("Note"))
                replace_encoding_area(x0, y0, x1, y1, '&');
            if (ImGui::MenuItem("Integer"))
                replace_encoding_area(x0, y0, x1, y1, '#');
            if (ImGui::MenuItem("Bits"))
                replace_encoding_area(x0, y0, x1, y1, '%');
            if (ImGui::MenuItem("Text"))
                replace_encoding_area(x0, y0, x1, y1, '\'');
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Enter note.."))
        {
            const char* notes[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G","G#","A","A#","B" };
            for (int i = 0; i < 12; i++)
            {
                if (ImGui::BeginMenu(notes[i]))
                {
                    if (ImGui::MenuItem("-1")) setcellvalue_area(x0, y0, x1, y1, 12 * 0 + i);
                    if (ImGui::MenuItem("0")) setcellvalue_area(x0, y0, x1, y1,  12 * 1 + i);
                    if (ImGui::MenuItem("1")) setcellvalue_area(x0, y0, x1, y1,  12 * 2 + i);
                    if (ImGui::MenuItem("2")) setcellvalue_area(x0, y0, x1, y1,  12 * 3 + i);
                    if (ImGui::MenuItem("3")) setcellvalue_area(x0, y0, x1, y1,  12 * 4 + i);
                    if (ImGui::MenuItem("4")) setcellvalue_area(x0, y0, x1, y1,  12 * 5 + i);
                    if (ImGui::MenuItem("5")) setcellvalue_area(x0, y0, x1, y1,  12 * 6 + i);
                    if (ImGui::MenuItem("6")) setcellvalue_area(x0, y0, x1, y1,  12 * 7 + i);
                    if (ImGui::MenuItem("7")) setcellvalue_area(x0, y0, x1, y1,  12 * 8 + i);
                    if (ImGui::MenuItem("8")) setcellvalue_area(x0, y0, x1, y1,  12 * 9 + i);
                    if (i < 8 && ImGui::MenuItem("9")) setcellvalue_area(x0, y0, x1, y1, 12 * 10 + i);
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }

        sprintf(temp, "Enter note %s", gNotestr[(int)gMidi_note]);
        if (ImGui::MenuItem(temp))
            setcellvalue_area(x0, y0, x1, y1, (int)gMidi_note);
        if (ImGui::BeginMenu("Bake.."))
        {
            if (ImGui::MenuItem("All"))
                bake_area(x0, y0, x1, y1, 0);
            if (ImGui::MenuItem("Sliders"))
                bake_area(x0, y0, x1, y1, 1);
            if (ImGui::MenuItem("Toggles"))
                bake_area(x0, y0, x1, y1, 2);
            if (ImGui::MenuItem("MIDI pots"))
                bake_area(x0, y0, x1, y1, 3);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Insert.."))
        {
            if (ImGui::BeginMenu("Rows above.."))
            {
                if (ImGui::MenuItem("1")) addrowcol(1, 0, 1);
                if (ImGui::MenuItem("2")) addrowcol(2, 0, 1);
                if (ImGui::MenuItem("3")) addrowcol(3, 0, 1);
                if (ImGui::MenuItem("4")) addrowcol(4, 0, 1);
                if (ImGui::MenuItem("5")) addrowcol(5, 0, 1);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Rows below.."))
            {
                if (ImGui::MenuItem("1")) addrowcol(1, 1, 1);
                if (ImGui::MenuItem("2")) addrowcol(2, 1, 1);
                if (ImGui::MenuItem("3")) addrowcol(3, 1, 1);
                if (ImGui::MenuItem("4")) addrowcol(4, 1, 1);
                if (ImGui::MenuItem("5")) addrowcol(5, 1, 1);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Columns left.."))
            {
                if (ImGui::MenuItem("1")) addrowcol(1, 0, 0);
                if (ImGui::MenuItem("2")) addrowcol(2, 0, 0);
                if (ImGui::MenuItem("3")) addrowcol(3, 0, 0);
                if (ImGui::MenuItem("4")) addrowcol(4, 0, 0);
                if (ImGui::MenuItem("5")) addrowcol(5, 0, 0);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Columns right.."))
            {
                if (ImGui::MenuItem("1")) addrowcol(1, 1, 0);
                if (ImGui::MenuItem("2")) addrowcol(2, 1, 0);
                if (ImGui::MenuItem("3")) addrowcol(3, 1, 0);
                if (ImGui::MenuItem("4")) addrowcol(4, 1, 0);
                if (ImGui::MenuItem("5")) addrowcol(5, 1, 0);

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Delete.."))
        {
            if (ImGui::MenuItem("Row(s)")) delrowcol(1);
            if (ImGui::MenuItem("Column(s)")) delrowcol(0);

            ImGui::EndMenu();
        }


        ImGui::EndPopup();
    }
}
