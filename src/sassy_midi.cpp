#include "sassy.h"

using namespace EvalFunc;

void midicallback(double /*deltatime*/, std::vector< unsigned char >* message, void* /*userData*/)
{
    if (gConfig.mMidiChannel != -1)
    {
        // Ignore messages sent to different midi channel in non-omni modes
        if (gConfig.mMidiChannel != ((*message)[0] & 0xf))
            return;
    }

    if (((*message)[0] & 0xf0) == 0xf0) // ignore sysex/control messages
        return;

    gMidimessage[0] = 0;
    for (unsigned int i = 0; i < message->size(); i++)
    {
        char temp[8];
        sprintf(temp, "%02X ", (*message)[i]);
        strcat(gMidimessage, temp);
    }

    char* s = gMidimessage + strlen(gMidimessage);

    if (((*message)[0] & 0xf0) == 0x80)
    {
        double v = (*message)[2] * (1 / 127.0);
        int note = (*message)[1];
        int channel = (*message)[0] & 0xf;
        sprintf(s, "| note off %d (%s), velocity %3.3f", note, gNotestr[note], v);
        if (gMidi_noteon)
        {
            gMidi_noteon--;
            gMidi_notevel = 0;
        }
        int slot = gMidi_poly_notetoindex[note | (channel << 7)];
        if (gMidi_poly_note[slot] == note)
        {
            gMidi_poly_noteon[slot] = 0;
            gMidi_poly_notevel[slot] = 0;
        }
        slot = gMidi_perchan_notetoindex[note + channel * 128];
        if (gMidi_perchan_note[slot] == note)
        {
            gMidi_perchan_noteon[slot] = 0;
            gMidi_perchan_notevel[slot] = 0;
        }
    }
    if (((*message)[0] & 0xf0) == 0x90)
    {
        double v = (*message)[2] * (1 / 127.0);
        int note = (*message)[1];
        int channel = (*message)[0] & 0xf;
        sprintf(s, "| note on %d (%s), velocity %3.3f", note, gNotestr[note], v);
        if ((*message)[2] == 0) // some keyboards send note off as silent note
        {
            if (gMidi_noteon)
            {
                gMidi_noteon--;
                gMidi_notevel = 0;
            }
            int slot = gMidi_poly_notetoindex[note | (channel << 7)];
            if (gMidi_poly_note[slot] == note)
            {
                gMidi_poly_noteon[slot] = 0;
                gMidi_poly_notevel[slot] = 0;
            }
            slot = gMidi_perchan_notetoindex[note + channel * 128];
            if (gMidi_perchan_note[slot] == note)
            {
                gMidi_perchan_noteon[slot] = 0;
                gMidi_perchan_notevel[slot] = 0;
            }
        }
        else
        {
            gMidi_noteon++;
            gMidi_notevel = v;
            gMidi_noteval = note_to_freq[note];
            gMidi_note = note;
            
            int slot = gMidi_poly_notetoindex[note | (channel << 7)];
            if (gMidi_poly_note[slot] == note)
            {
                // Another note on for the same note?
            }
            else
            {
                slot = 0;
                while (slot < 128 && gMidi_poly_noteon[slot] != 0) slot++;
            }
            if (slot < 128)
            {
                gMidi_poly_noteon[slot] = 1;
                gMidi_poly_note[slot] = note;
                gMidi_poly_notevel[slot] = v;
                gMidi_poly_noteval[slot] = note_to_freq[note];
                gMidi_poly_channel[slot] = channel;
                gMidi_poly_notetoindex[note | (channel << 7)] = slot;
            }

            slot = gMidi_perchan_notetoindex[note + channel * 128];
            if (gMidi_perchan_note[slot] == note)
            {
                // Another note on for the same note?
            }
            else
            {
                slot = 0;
                while (slot < 128 && gMidi_perchan_noteon[slot + channel * 128] != 0) slot++;
            }
            if (slot < 128)
            {
                slot = slot + channel * 128;
                gMidi_perchan_noteon[slot] = 1;
                gMidi_perchan_note[slot] = note;
                gMidi_perchan_notevel[slot] = v;
                gMidi_perchan_noteval[slot] = note_to_freq[note];
                gMidi_perchan_notetoindex[note + channel * 128] = slot;
            }
        }
    }
    if (((*message)[0] & 0xf0) == 0xa0)
    {
        double v = (*message)[2] * (1 / 127.0);
        sprintf(s, "| poly aftertouch %d (%s), pressure %3.3f", (*message)[1], gNotestr[(*message)[1]], v);
        gMidi_notevel = v;
        int slot = gMidi_poly_notetoindex[((*message)[1] | (((*message)[0] & 0xf) << 7))];
        if (gMidi_poly_note[slot] == (*message)[1])
        {
            gMidi_poly_notevel[slot] = v;
        }
        slot = gMidi_perchan_notetoindex[((*message)[1] | (((*message)[0] & 0xf) << 7))];
        if (gMidi_perchan_note[slot] == (*message)[1])
        {
            gMidi_perchan_notevel[slot] = v;
        }
    }
    if (((*message)[0] & 0xf0) == 0xb0)
    {
        double v = (*message)[2] * (1 / 127.0);
        sprintf(s, "| pot %d, value %3.3f", (*message)[1], v);
        gMidi_pot[(*message)[1]] = v;
        gMidi_pot_changed[(*message)[1]] = 1;
    }
    if (((*message)[0] & 0xf0) == 0xc0)
    {
        sprintf(s, "| program change %d", (*message)[1]);
        gMidi_prog = (*message)[1];
    }
    if (((*message)[0] & 0xf0) == 0xd0)
    {
        double v = (*message)[1] * (1 / 127.0);
        sprintf(s, "| mono aftertouch, pressure %3.3f", v);
        gMidi_notevel = v;
    }
    if (((*message)[0] & 0xf0) == 0xe0)
    {
        double v = (((*message)[2] << 7) | ((*message)[1])) * (1.0 / 16384.0);
        sprintf(s, "| pitch bend value %3.3f", v);
        gMidi_pitch = v;
    }
    /*
    if (((*message)[0] & 0xf0) == 0xf0)
    {
        sprintf(s, "| sysex / control");
    }
    */
}

void addactivemidiout(int v)
{
    gActiveMidiout[gActiveMidiouts] = v;
    gActiveMidiouts++;
}

void removeactivemidiout(int v)
{
    // Not optimal by any means, but the active midi outs count is
    // not likely to be very high.
    for (int i = 0; i < gActiveMidiouts; i++)
    {
        if (gActiveMidiout[i] == v)
        {
            if (gActiveMidiouts > 1)
            {
                gActiveMidiout[i] = gActiveMidiout[gActiveMidiouts - 1];
            }
            gActiveMidiouts--;
            return;
        }
    }


}

void midioutscan()
{
    double now = gettime();
    unsigned char data[3];    
    for (int a = 0; gMidiCredits && a < gActiveMidiouts; a++)
    {
        int i = gActiveMidiout[a];
        if (gOutmidivel[i] != 0 && gOutmidiupdate[i] != now)
        {
            if (gMidiCredits >= 1)
            {
                gMidiCredits--;
                // note not updated, send note off            
                data[0] = (unsigned char)(0x80 + (i / 128));
                data[1] = (unsigned char)i;
                data[2] = gOutmidivel[i];
                gRtmidi_out->sendMessage(data, 3);
                gOutmidivel[i] = 0;
                if (gActiveMidiouts > 1)
                {
                    gActiveMidiout[a] = gActiveMidiout[gActiveMidiouts - 1];
                    a--;
                }
                gActiveMidiouts--;
            }
        }
    }
}

namespace EvalFunc
{

    double midipot(double v, double index, int cell, int memofs)
    {
        int* data = (int*)(gCelldata[cell].mDynmem + memofs);
        int idx = (int)index;
        if (idx < 0 || idx > 127)
            return 0;

        if (!*data)
        {
            gMidi_pot[idx] = (float)v;
            *data = 1;
        }
        else
        {
            if (gMidi_pot_changed[idx])
                gMidi_pot_cell[idx] = cell;
        }

        return gMidi_pot[idx];
    }

    double sliderpot(double v, double index, int cell, int memofs)
    {
        int idx = (int)index;
        if (idx < 0 || idx > 127)
            return 0;

        int* data = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(float));
        if (!*data)
        {
            *(float*)(gCelldata[cell].mDynmem + memofs) = (float)v;
            gMidi_pot[idx] = (float)v;
            *data = 1;
        }
        else
        {
            if (gMidi_pot_changed[idx])
                gMidi_pot_cell[idx] = cell;
        }
        return *(float*)(gCelldata[cell].mDynmem + memofs);
    }


    double togglepot(double v, double pot, int cell, int memofs)
    {
        int idx = (int)pot;
        if (idx < 0 || idx > 127)
            return 0;
        double* prev = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) + sizeof(int));
        int* data = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        if (!*data)
        {
            *(int*)(gCelldata[cell].mDynmem + memofs) = (v > 0.5) ? 1 : 0;
            gMidi_pot[idx] = (float)v;
            *data = 1;
            *prev = v;
        }
        else
        {
            if (gMidi_pot_changed[idx])
            {
                if (*prev < 0.5 && gMidi_pot[idx] > 0.5)
                {
                    gMidi_pot_cell[idx] = cell;
                    *(int*)(gCelldata[cell].mDynmem + memofs) = 1 - *(int*)(gCelldata[cell].mDynmem + memofs);
                }
                *prev = gMidi_pot[idx];
            }
        }
        return *(int*)(gCelldata[cell].mDynmem + memofs) ? 1.0 : 0.0;
    }

    double midiout(double note, double vel)
    {
        return midioutc(0, note, vel);
    }
    
    double midioutc(double channel, double note, double vel)
    {
        int ch = (int)channel;
        if (ch < 0 || ch > 15) return note;
        int n = (int)note;
        int v = (int)(vel * 127);
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        if (n < 0 || n > 127)
            return note;
        n += ch * 128;
        gOutmidiupdate[n] = gettime();
        if (gOutmidivel[n] == v) // unchanged
            return note;

        if (gMidiCredits < 1)
        {
            return note;
        }
        gMidiCredits--;

        unsigned char data[3];

        // case 1: note on
        if (gOutmidivel[n] == 0)
        {
            gOutmidivel[n] = (unsigned char)v;
            data[0] = (unsigned char)(0x90 + ch);
            data[1] = (unsigned char)n;
            data[2] = (unsigned char)v;
            gRtmidi_out->sendMessage(data, 3);
            addactivemidiout(n);
            return note;
        }
        // case 2: note off
        if (v == 0)
        {
            gOutmidivel[n] = (unsigned char)v;
            data[0] = (unsigned char)(0x80 + ch);
            data[1] = (unsigned char)n;
            data[2] = (unsigned char)v;
            gRtmidi_out->sendMessage(data, 3);
            removeactivemidiout(n);
            return note;
        }
        // case 3: aftertouch
        gOutmidivel[n] = (unsigned char)v;
        data[0] = (unsigned char)(0xa0 + ch); // polyphonic aftertouch
        data[1] = (unsigned char)n;
        data[2] = (unsigned char)v;
        gRtmidi_out->sendMessage(data, 3);
        return note;
    }


    double midioutpitch(double pitch)
    {
        return midioutpitchc(0, pitch);
    }

    double midioutpitchc(double channel, double pitch)
    {
        int ch = (int)channel;
        if (ch < 0 || ch > 15) return pitch;
        int p = (int)(pitch * 16383);
        if (p < 0) p = 0;
        if (p > 16383) p = 16383;
        if (gOutmidipitch[ch] == p) // unchanged
            return pitch;

        if (gMidiCredits < 1)
        {
            return pitch;
        }
        gMidiCredits--;

        gOutmidipitch[ch] = p;
        unsigned char data[3];
        data[0] = (unsigned char)(0xe0 + ch);
        data[1] = (p >> 0) & 0x7f;
        data[2] = (p >> 7) & 0x7f;
        gRtmidi_out->sendMessage(data, 3);
        return pitch;
    }

    double midioutpot(double value, double index)
    {
        return midioutpotc(0, value, index);
    }

    double midioutpotc(double channel, double value, double index)
    {
        int ch = (int)channel;
        if (ch < 0 || ch > 15) return value;
        int n = (int)index;
        int v = (int)(value * 127);
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        if (n < 0 || n > 127)
            return value;
        n += ch * 128;
        if (gOutmidipot[n] == v) // unchanged
            return value;

        if (gMidiCredits < 1)
        {
            return value;
        }
        gMidiCredits--;

        gOutmidipot[n] = v;

        unsigned char data[3];
        data[0] = (unsigned char)(0xb0 + ch);
        data[1] = (unsigned char)n;
        data[2] = (unsigned char)v;
        gRtmidi_out->sendMessage(data, 3);
        return value;
    }


    double midioutprog(double prog)
    {
        return midioutprogc(0, prog);
    }

    double midioutprogc(double channel, double prog)
    {
        int ch = (int)channel;
        if (ch < 0 || ch > 15) return prog;
        int n = (int)prog;
        if (n < 0) n = 0;
        if (n > 127) n = 127;
        if (gOutmidiprog[ch] == n) // unchanged
            return prog;

        if (gMidiCredits < 1)
        {
            return prog;
        }
        gMidiCredits--;

        gOutmidiprog[ch] = n;

        unsigned char data[3];
        data[0] = (unsigned char)(0xc0 + ch);
        data[1] = (unsigned char)n;
        data[2] = 0;
        gRtmidi_out->sendMessage(data, 2);
        return prog;
    }

    double midioutraw(double gate, double a, double b, double c)
    {
        if (gate < 0.5) return gate;

        if (gMidiCredits < 1)
        {
            return gate;
        }
        gMidiCredits--;

        unsigned char data[3];
        data[0] = (unsigned char)a;
        data[1] = (unsigned char)b;
        data[2] = (unsigned char)c;
        gRtmidi_out->sendMessage(data, 3);
        return gate;
    }

    double midivalv(double ch)
    {
        int c = (int)ch;
        if (c < 0 || c > 127) return 0;
        return gMidi_poly_noteval[c];
    }

    double midinotev(double ch)
    {
        int c = (int)ch;
        if (c < 0 || c > 127) return 0;
        return gMidi_poly_note[c];
    }

    double midivelv(double ch)
    {
        int c = (int)ch;
        if (c < 0 || c > 127) return 0;
        return gMidi_poly_notevel[c];
    }

    double midionv(double ch)
    {
        int c = (int)ch;
        if (c < 0 || c > 127) return 0;
        return gMidi_poly_noteon[c] ? 1.0 : 0.0;
    }

    double midichannel(double ch)
    {
        int c = (int)ch;
        if (c < 0 || c > 127) return 0;
        return gMidi_poly_channel[c];
    }

    double midival()
    {
        return gMidi_noteval;
    }

    double midinote()
    {
        return gMidi_note;
    }

    double midivel()
    {
        return gMidi_notevel;
    }

    double midion()
    {
        return (gMidi_noteon > 0) ? 1.0 : 0.0;
    }

    double midipitch()
    {
        return gMidi_pitch;
    }

    double midiprog()
    {
        return gMidi_prog;
    }

    double midivalc(double nn, double ch)
    {
        int n = (int)nn;
        int c = (int)ch;
        if (n < 0 || c < 0 || c > 15 || n > 127) return 0;
        return gMidi_perchan_noteval[c * 128 + n];
    }

    double midinotec(double nn, double ch)
    {
        int n = (int)nn;
        int c = (int)ch;
        if (n < 0 || c < 0 || c > 15 || n > 127) return 0;
        return gMidi_perchan_note[c * 128 + n];
    }

    double midivelc(double nn, double ch)
    {
        int n = (int)nn;
        int c = (int)ch;
        if (n < 0 || c < 0 || c > 15 || n > 127) return 0;
        return gMidi_perchan_notevel[c * 128 + n];
    }

    double midionc(double nn, double ch)
    {
        int n = (int)nn;
        int c = (int)ch;
        if (n < 0 || c < 0 || c > 15 || n > 127) return 0;
        return gMidi_perchan_noteon[c * 128 + n];
    }
};