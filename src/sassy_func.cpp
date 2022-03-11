#include "sassy.h"

extern "C"
{
#include "ayumi/ayumi.h"
#define STB_HEXWAVE_IMPLEMENTATION
#include "stb/stb_hexwave.h"
}


char* mystrdup(const char* s)
{
    size_t len = strlen(s);
    char* t = new char[len + 1];
    memcpy(t, s, len + 1);
    return t;
}


extern "C" unsigned char akwfdata[];
extern "C" unsigned int akwfdata_size;


namespace EvalFunc
{

    double gettime()
    {
        return gTime;
    }

    double dt()
    {
        return 1.0 / gSamplerate;
    }


    double paranoise(double oct0, double oct1, double oct2, double oct3, double oct4, double oct5, double oct6, double oct7, double oct8, double oct9, int cell, int memofs)
    {
        double parm[10] = { oct0,oct1, oct2, oct3,oct4,oct5, oct6,oct7, oct8,oct9 };
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) * 10);
        int* index = (int*)(gCelldata[cell].mDynmem + memofs);
        double output = 0;
        double scale = 0;
        for (int i = 0; i < 10; i++)
        {
            index[i]++;
            if (index[i] > (1 << i))
            {
                index[i] = 0;
                data[i] = noise();
            }
            output += parm[i] * data[i];
            scale += parm[i];
        }
        if (scale)
            output /= scale;
        return output;
    }

    double allpass(double v, double t, double feedback, int cell, int memofs)
    {
        if (!isnormal(v)) v = 0; // kill nans
        if (!isnormal(t)) t = 0;
        if (feedback != feedback) feedback = 0;
        int cycle = (int)(gSamplerate * t);
        if (cycle < 1) cycle = 1;
        if (cycle > gSamplerate) cycle = gSamplerate;
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        int* index = (int*)(gCelldata[cell].mDynmem + memofs);
        double looped = data[(*index + gSamplerate - cycle) % gSamplerate];
        double output = -v + looped;
        data[*index] = v + (looped * feedback);

        *index = (*index + 1) % gSamplerate;

        return output;
    }

    double comb(double v, double t, double damp, double feedback, int cell, int memofs)
    {
        if (!isnormal(v)) v = 0; // kill nans
        if (!isnormal(t)) t = 0;
        if (damp != damp) damp = 0;
        if (feedback != feedback) feedback = 0;
        int cycle = (int)(gSamplerate * t);
        if (cycle < 1) cycle = 1;
        if (cycle > gSamplerate) cycle = gSamplerate;
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) + sizeof(double));
        int* index = (int*)(gCelldata[cell].mDynmem + memofs);
        double* filterstore = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        double output = data[(*index + gSamplerate - cycle) % gSamplerate];

        *filterstore = (output * (1 - damp)) + (*filterstore * damp);

        data[*index] = v + *filterstore * feedback;

        *index = (*index + 1) % gSamplerate;

        return output;

    }

    // Based on public domain code by "Jezar at Dreampoint"
    double reverb(double v, double roomsize, double damp, double freeze, int cell, int memofs)
    {
        const double fixedgain = 0.015;
        const double scaledamp = 0.4;
        const double scaleroom = 0.28;
        const double offsetroom = 0.7;
        double scale = gSamplerate / 44100.0;
        // TODO: move this mess to config or something
        const int comb[] = { (int)(1116 * scale), (int)(1188 * scale), (int)(1277 * scale),  (int)(1356 * scale), (int)(1422 * scale), (int)(1491 * scale),  (int)(1557 * scale),  (int)(1617 * scale) };
        const int ap[] = { (int)(556 * scale), (int)(441 * scale), (int)(341 * scale), (int)(225 * scale) };

        int* index = (int*)(gCelldata[cell].mDynmem + memofs);
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) * 12);

        double input = v * fixedgain;

        damp *= scaledamp;
        roomsize = roomsize * scaleroom + offsetroom;

        if (freeze > 0.5)
        {
            roomsize = 1;
            damp = 0;
            input = 0;
        }

        double out = 0;
        int ofs = 0;
        for (int i = 0; i < 8; i++)
        { // comb filters
            double output = data[index[i] + ofs + 1];
            data[ofs] = output * (1 - damp) + data[ofs] * damp;
            data[index[i] + ofs + 1] = input + data[ofs] * roomsize;
            index[i]++;
            if (index[i] >= comb[i]) index[i] = 0;
            out += output;
            ofs += comb[i] + 1;
        }

        for (int i = 0; i < 4; i++)
        { // allpass filters
            double bufout = data[index[i + 8] + ofs];
            double output = -out + bufout;
            data[index[i + 8] + ofs] = out + (bufout * 0.5);
            index[i + 8]++;
            if (index[i + 8] >= ap[i]) index[i + 8] = 0;
            out = output;
            ofs += ap[i];
        }

        return out;
    }


    double adsr(double a, double d, double s, double r, double gate, int cell, int memofs)
    {
        double* statestart = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        double* level = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) + sizeof(double));
        int* state = (int*)(gCelldata[cell].mDynmem + memofs);
        int newstate = (gate > 0.5) || (gate < -0.5);

        double now = gettime();
        if (newstate != *state)
        {
            double dt = now - *statestart; // old dt
            *statestart = now;
            *state = newstate;
            if (newstate)
            {
                if (gate < -0.5)
                {
                    *level = 0;
                }
                else
                    if (dt < r)
                    {
                        // In release stage, calculate remaining level
                        *level = (1.0f - dt / r) * *level;
                    }
                // *level is current output level. Move start time back so
                // attack starts from the current level.
                *statestart -= *level * a;
            }
        }
        double dt = now - *statestart;
        if (*state)
        {
            // attack-decay-sustain
            if (dt < a)
            {
                *level = dt / a;
                return *level;
            }
            dt -= a;
            if (dt < d)
            {
                *level = 1.0 - (dt / d) * (1.0 - s);
                return *level;
            }
            *level = s;
            return s;
        }
        // release
        if (dt > r)
        {
            *level = 0;
            return 0;
        }
        return (1.0f - dt / r) * *level;
    }

    double filter(double v, double f, double r, double t, double c, int cell, int memofs)
    {
        if (!isnormal(v)) v = 0; // kill nans
        if (!isnormal(c)) c = 1;
        if (!isnormal(f) || f < 1) f = 1;
        if (!isnormal(r) || r < 0.1) r = 0.1;
        if (f > gSamplerate * 0.49) f = gSamplerate * 0.49; // slightly below nyqvist
        if (t < 0 || t > 2) t = 0;
        c = (int)c;
        if (c < 1) c = 1;
        if (c > 6) c = 6;
        int mode = (int)t;

        // state = mX0, mX1, mY0, mY1, x

        double omega = ((2.0 * M_PI * f) / gSamplerate);
        double sin_omega = sin(omega);
        double cos_omega = cos(omega);
        double alpha = sin_omega / (2.0 * r);
        double scalar = 1.0 / (1.0 + alpha);
        double a0, a1, a2, b1, b2;
        switch (mode)
        {
        default:
        case 0: // lp
            a0 = 0.5 * (1.0 - cos_omega) * scalar;
            a1 = (1.0 - cos_omega) * scalar;
            a2 = a0;
            b1 = -2.0 * cos_omega * scalar;
            b2 = (1.0 - alpha) * scalar;
            break;
        case 1: // hp
            a0 = 0.5 * (1.0 + cos_omega) * scalar;
            a1 = -(1.0 + cos_omega) * scalar;
            a2 = a0;
            b1 = -2.0 * cos_omega * scalar;
            b2 = (1.0 - alpha) * scalar;
            break;
        case 2: // bp
            a0 = alpha * scalar;
            a1 = 0;
            a2 = -a0;
            b1 = -2.0 * cos_omega * scalar;
            b2 = (1.0 - alpha) * scalar;
            break;
        }

        double res = 0;

        for (int i = 0; i < c; i++)
        {
            int* ticktock = (int*)(gCelldata[cell].mDynmem + memofs + (sizeof(double) * 5 + sizeof(int)) * i);
            double* state = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) + (sizeof(double) * 5 + sizeof(int)) * i);
            double* x = state + 0;
            double* x1 = state + 1;
            double* x2 = state + 2;
            double* y1 = state + 3;
            double* y2 = state + 4;
            if (*ticktock == 0)
            {
                // tick
                *x = v;
                *y2 = (a0 * *x) + (a1 * *x1) + (a2 * *x2) - (b1 * *y1) - (b2 * *y2);
                res = *y2;
            }
            else
            {
                // tock
                *x2 = v;
                *y1 = (a0 * *x2) + (a1 * *x) + (a2 * *x1) - (b1 * *y2) - (b2 * *y1);
                res = *y1;
            }
            v = res;
            *ticktock ^= 1;
            *x1 = *x2;
            *x2 = *x;
        }
        return res;
    }

    double timegate(double v, int cell, int memofs)
    {
        double* t0 = (double*)(gCelldata[cell].mDynmem + memofs);
        double now = gettime();
        if (v < 0.5)
        {
            *t0 = now;
        }
        return now - *t0;
    }

    double trigger(double v, double mode, int cell, int memofs)
    {
        double* prev = (double*)(gCelldata[cell].mDynmem + memofs);
        int m = (int)mode;
        double res = 0;
        if ((m == 0 || m == 2) && *prev <= 0 && v > 0)
            res = 1;
        if ((m == 1 || m == 2) && *prev >= 0 && v < 0)
            res = 1;
        *prev = v;
        return res;
    }

    double step(double s, int cell, int memofs)
    {
        double* t0 = (double*)(gCelldata[cell].mDynmem + memofs);
        if (s != s) s = 0;
        *t0 += gConfig.mDt * s;
        return *t0;
    }

    double stepg(double s, double gate, int cell, int memofs)
    {
        double* t0 = (double*)(gCelldata[cell].mDynmem + memofs);
        if (gate < 0.5)
        {
            *t0 = 0;
        }
        else
        {
            if (s != s) s = 0;
            *t0 += gConfig.mDt * s;
        }
        return *t0;
    }


    double plotxy(double a, int cell, int memofs)
    {
        float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        int* graphindex = (int*)(gCelldata[cell].mDynmem + memofs);
        graphdata[*graphindex * 2 + 0] = (float)a;
        graphdata[*graphindex * 2 + 1] = graphdata[((*graphindex + 1023) % 1024) * 2 + 0];
        *graphindex = (*graphindex + 1) % (1024);
        return a;
    }

    double plotxy2(double a, double b, int cell, int memofs)
    {
        float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        int* graphindex = (int*)(gCelldata[cell].mDynmem + memofs);
        graphdata[*graphindex * 2 + 0] = (float)a;
        graphdata[*graphindex * 2 + 1] = (float)b;
        *graphindex = (*graphindex + 1) % (1024);
        return a;
    }


    double graph(double v, int cell, int memofs)
    {
        float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int) * 2);
        int* graphindex = (int*)(gCelldata[cell].mDynmem + memofs);
        graphdata[*graphindex] = (float)v;
        *graphindex = (*graphindex + 1) % (1 << 17);
        return v;
    }

    double fft(double v, int cell, int memofs)
    {
        float* graphdata = (float*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        int* graphindex = (int*)(gCelldata[cell].mDynmem + memofs);
        graphdata[*graphindex] = (float)v;
        *graphindex = (*graphindex + 1) % (FFTSIZE);
        return v;
    }

    double bar(double v, int cell, int memofs)
    {
        float* data = (float*)(gCelldata[cell].mDynmem + memofs);
        *data = (float)v;
        return v;
    }

    double pie(double v, int cell, int memofs)
    {
        float* data = (float*)(gCelldata[cell].mDynmem + memofs);
        *data = (float)v;
        return v;
    }

    double slider(double v, int cell, int memofs)
    {
        int* data = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(float));
        if (!*data)
        {
            *(float*)(gCelldata[cell].mDynmem + memofs) = (float)v;
            *data = 1;
        }
        return *(float*)(gCelldata[cell].mDynmem + memofs);
    }

    double encoder(double v, int cell, int memofs)
    {
        int* data = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(float));
        if (!*data)
        {
            *(float*)(gCelldata[cell].mDynmem + memofs) = (float)v;
            *data = 1;
        }
        return *(float*)(gCelldata[cell].mDynmem + memofs);
    }

    double button(int cell, int memofs)
    {
        return *(int*)(gCelldata[cell].mDynmem + memofs);
    }

    double toggle(double v, int cell, int memofs)
    {
        int* data = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        if (!*data)
        {
            *(bool*)(gCelldata[cell].mDynmem + memofs) = v > 0.5;
            *data = 1;
        }
        return *(bool*)(gCelldata[cell].mDynmem + memofs) ? 1.0 : 0.0;
    }


    double output(double val)
    {
        float out = (float)val;
        if (out > 1) out = 1;
        if (out < -1) out = -1;
        gOutputSample[0] = out;
        gOutputSample[1] = out;
        return val;
    }

    double output2(double val1, double val2)
    {
        float out1 = (float)val1;
        if (out1 > 1) out1 = 1;
        if (out1 < -1) out1 = -1;
        float out2 = (float)val2;
        if (out2 > 1) out2 = 1;
        if (out2 < -1) out2 = -1;
        gOutputSample[0] = out1;
        gOutputSample[1] = out2;
        return val1;
    }

    double in()
    {
        return (gInputSample[0] + gInputSample[1]) * 0.5;
    }

    double in2(double channel)
    {
        if (channel < 0.5)
            return gInputSample[0];
        return gInputSample[1];
    }


    double getvar(int var)
    {
        if (var < 1 || var >= MAXVAR)
            return nan("0");
        return gCellvalue[var - 1];
    }

    double delay(double v, double t, int cell, int memofs)
    {
        int cycle = (int)(gSamplerate * t);
        if (cycle < 1) cycle = 1;
        if (cycle > gSamplerate) cycle = gSamplerate;
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        int* index = (int*)(gCelldata[cell].mDynmem + memofs);
        if (!isnormal(v)) v = 0; // kill nans
        data[*index] = v;
        *index = (*index + 1) % gSamplerate;

        return data[(*index + gSamplerate - cycle) % gSamplerate];
    }

    double hold(double v, double t, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 0);
        double* t0 = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 1);
        double now = gettime();

        if (!isnormal(v)) v = 0; // kill nans

        if (now > *t0 + t)
        {
            *t0 = now;
            *data = v;
        }

        return *data;
    }

    double holdl(double v, double t, double l, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 0);
        double* t0 = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 1);
        double* prev = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 2);
        double now = gettime();

        if (!isnormal(v)) v = 0; // kill nans
        if (!isnormal(t)) t = 0; // kill nans
        if (!isnormal(l)) l = 0; // kill nans    

        if (now > *t0 + t)
        {
            *prev = *data;
            *t0 = now;
            *data = v;
        }

        if (l == 0 || t == 0)
            return *data;
        if (l == 1)
            return *prev + (*data - *prev) * ((now - *t0) / t);
        if (l == -1)
            return *data + (*prev - *data) * ((now - *t0) / t);
        return *prev + (*data - *prev) * l;
    }


    double latch(double v, double l, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 0);
        double* clock = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 1);

        if (!isnormal(v)) v = 0; // kill nans

        if (l > 0.5 && *clock < 0.5)
        {
            *data = v;
        }
        *clock = l;

        return *data;
    }

    double rubberband(double val, double str, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs);

        if (str != -1)
            *data = (val + *data * str) / (str + 1);

        if (!isnormal(*data)) *data = 0; // kill nans

        return *data;
    }


    double noise()
    {
        return gRandom.genrand_real1();
    }


    void celltorowcol(double cell, int& row, int& col)
    {
        int c = (int)cell;
        row = (c - 1) / 27;
        col = (c - 1) % 27;
    }

    double tablemin(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        double v = 0;
        int first = 1;
        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    if (first)
                    {
                        first = 0;
                        v = gCellvalue[i * 27 + j];
                    }
                    if (gCellvalue[i * 27 + j] < v)
                        v = gCellvalue[i * 27 + j];
                }
            }
        }
        return v;
    }

    double tablemax(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        double v = 0;
        int first = 1;
        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    if (first)
                    {
                        first = 0;
                        v = gCellvalue[i * 27 + j];
                    }
                    if (gCellvalue[i * 27 + j] > v)
                        v = gCellvalue[i * 27 + j];
                }
            }
        }
        return v;
    }

    double tableaverage(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        double v = 0;
        int count = 0;

        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    count++;
                    v += gCellvalue[i * 27 + j];
                }
            }
        }
        if (count == 0)
            return 0;
        return v / count;
    }

    double tablesum(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        double v = 0;

        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    v += gCellvalue[i * 27 + j];
                }
            }
        }
        return v;
    }

    double tableselect(double a, double b, double v)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;

        int count = (row2 - row1) * (col2 - col1);
        if (count == 0)
            return 0;

        int selected = (int)v % count;

        int row = (selected / (col2 - col1)) + row1;
        int col = (selected % (col2 - col1)) + col1;

        hilight(row + 1, col + 1, rgb(0.1f, 0.3f, 0.1f));
        //    gCelldata[row * 27 + col].hilighttime = gettime();

        return getvar(row * 27 + col + 1);
    }

    double tableselectv(double a, double b, double v)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;

        int count = (row2 - row1) * (col2 - col1);
        if (count == 0)
            return 0;

        int selected = (int)v % count;

        int row = (selected % (row2 - row1)) + row1;
        int col = (selected / (row2 - row1)) + col1;

        hilight(row + 1, col + 1, rgb(0.1f, 0.3f, 0.1f));
        //    gCelldata[row * 27 + col].hilighttime = gettime();

        return getvar(row * 27 + col + 1);
    }

    double tablecount(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        int count = 0;

        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    count++;
                }
            }
        }
        return count;
    }

    double tablefind(double a, double b, double v)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        int n = 0;

        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (gCellvalue[i * 27 + j] == v)
                {
                    return n;
                }
                n++;
            }
        }
        return nan("0");
    }

    double tablefindv(double a, double b, double v)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        int n = 0;

        for (int j = col1; j < col2; j++)
        {
            for (int i = row1; i < row2; i++)
            {
                if (gCellvalue[i * 27 + j] == v)
                {
                    return n;
                }
                n++;
            }
        }
        return nan("0");
    }

    double tableproduct(double a, double b)
    {
        int row1, col1, row2, col2;
        celltorowcol(a, row1, col1);
        celltorowcol(b, row2, col2);
        if (row1 > row2) { int t = row1; row1 = row2; row2 = t; }
        if (col1 > col2) { int t = col1; col1 = col2; col2 = t; }
        row2 += 1;
        col2 += 1;
        double v = 1;
        int count = 0;

        for (int i = row1; i < row2; i++)
        {
            for (int j = col1; j < col2; j++)
            {
                if (!gCelldata[i * 27 + j].mText && (gCellvalue[i * 27 + j] == gCellvalue[i * 27 + j]))
                {
                    count++;
                    v *= gCellvalue[i * 27 + j];
                }
            }
        }
        if (count == 0)
            return 0;
        return v;
    }

    double hilight(double row, double column, double color)
    {
        int r = (int)row - 1;
        int c = (int)column - 1;
        if (r < 0 || r >= MAXROWS || c < 0 || c > 26)
            return nan("0");
        gCelldata[r * 27 + c].mHilighttime = gettime();
        gCelldata[r * 27 + c].mHilightcolor = (int)color;
        return 0;
    }


    double replace(double row, double column, double value)
    {
        int r = (int)row - 1;
        int c = (int)column - 1;
        if (r < 0 || r >= MAXROWS || c < 0 || c > 26)
            return nan("0");
        setcellvalue(r * 27 + c, value);
        return value;
    }

    double lookup(double row, double column)
    {
        int r = (int)row - 1;
        int c = (int)column - 1;
        if (r < 0 || r >= MAXROWS || c < 0 || c > 26)
            return nan("0");
        return gCellvalue[r * 27 + c];
    }

    double dc(double v, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(double) + sizeof(int));
        int* index = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(double));
        double* sum = (double*)(gCelldata[cell].mDynmem + memofs);
        int cycle = gSamplerate;
        *sum -= data[*index];
        data[*index] = v;
        *sum += v;
        *index = (*index + 1) % cycle;
        return *sum / cycle;
    }

    double drunkardswalk(double step, double gate, int cell, int memofs)
    {
        double* state = (double*)(gCelldata[cell].mDynmem + memofs);
        int* prev = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(double));
        if (gate < 0.5)
            return *state;

        int choice = gRandom.genrand_int31() % 4;
        if (choice == 3) choice = *prev;
        *prev = choice;
        switch (choice)
        {
        case 0: *state -= step; break;
        case 1: *state += step; break;
        }

        if (*state > 1) *state = 1;
        if (*state < 0) *state = 0;
        return *state;
    }

    double cpu()
    {
        return gCPUTimePerSample;
    }

    double probe(double c, double v)
    {
        int ch = (int)c;
        if (c < 0 || c > 3)
            return v;
        gProbe[ch] = (float)v;
        return v;
    }

    double img(char* fn, double xi, double yi, int cell, int memofs)
    {
        double* coords = (double*)(gCelldata[cell].mDynmem + memofs);
        ImgResource** res = (ImgResource**)(gCelldata[cell].mDynmem + memofs + sizeof(double) * 2);
        if (!isnormal(xi)) xi = 0; // kill nan
        if (!isnormal(yi)) yi = 0;
        coords[0] += xi;
        coords[1] += yi;
        coords[0] -= (int)coords[0];
        coords[1] -= (int)coords[1];
        if (coords[0] < 0) coords[0] += 1;
        if (coords[1] < 0) coords[1] += 1;
        if (!*res)
        {
            *res = (ImgResource*)getResource(fn, 0, 1);
            if (!*res) return 0;
        }
        if (*res == (ImgResource*)-1)
            return 0;

        unsigned int pos = (unsigned int)(coords[1] * 256) * 256 + (unsigned int)(coords[0] * 256);

        double sx = coords[0] * 256; sx -= (int)sx;
        double sy = coords[1] * 256; sy -= (int)sy;

        double a = (1 - sx) * (1 - sy) * ((*res)->imgdata[pos & 0xffff] * (1 / 255.0) - 0.5);
        double b = (sx) * (1 - sy) * ((*res)->imgdata[(pos + 1) & 0xffff] * (1 / 255.0) - 0.5);
        double c = (1 - sx) * (sy) * ((*res)->imgdata[(pos + 256) & 0xffff] * (1 / 255.0) - 0.5);
        double d = (sx) * (sy) * ((*res)->imgdata[(pos + 257) & 0xffff] * (1 / 255.0) - 0.5);
        return a + b + c + d;
    }

    double loadwav(char* fn, int cell, int memofs)
    {
        return loadwavc(fn, 0, cell, memofs);
    }

    double loadwavc(char* fn, double channel, int cell, int memofs)
    {
        WavResource** res = (WavResource**)(gCelldata[cell].mDynmem + memofs);
        if (!*res)
        {
            int ch = (int)channel;
            *res = (WavResource*)getResource(fn, ch, 0);
            if (!*res) return 0;
        }
        if (*res == (WavResource*)-1)
            return 0;
        return (*res)->handle;
    }

    double bara(double a, double b, int cell, int memofs)
    {
        int* coords = (int*)(gCelldata[cell].mDynmem + memofs);

        coords[0] = (int)a;
        coords[1] = (int)b;

        // actual work is done in ui
        return 0;
    }

    double piea(double a, double b, int cell, int memofs)
    {
        int* coords = (int*)(gCelldata[cell].mDynmem + memofs);

        coords[0] = (int)a;
        coords[1] = (int)b;

        // actual work is done in ui
        return 0;
    }

    double len(double v)
    {
        int handle = (int)v;
        if ((handle & ((int)'S' << 24)) != ((int)'S' << 24))
            return 0;
        handle &= 0xffffff;
        if (handle >= MAX_HANDLES)
            return 0;
        if (gWavHandle[handle])
        {
            return gWavHandle[handle]->frames / (double)gSamplerate;
        }
            
        return 0;
    }

    double channels(double v)
    {
        int handle = (int)v;
        if ((handle & ((int)'S' << 24)) != ((int)'S' << 24))
            return 0;
        handle &= 0xffffff;
        if (handle >= MAX_HANDLES)
            return 0;
        if (gWavHandle[handle])
        {
            return gWavHandle[handle]->channels;
        }
        return 0;
    }

    double play(double v, double trigger, int cell, int memofs)
    {
        int* pos = (int*)(gCelldata[cell].mDynmem + memofs);
        double *prevtrig = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));

        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        if (*prevtrig < 0.5 && trigger > 0.5)
            *pos = 1;
        *prevtrig = trigger;
        if (*pos == 0)
            return 0;

        if (*pos >= gWavHandle[handle]->frames - 1)
            *pos = gWavHandle[handle]->frames - 1;

        double s = gWavHandle[handle]->data[*pos - 1];
        (*pos)++;

        return s;
    }

    double playloop(double v, double type, double gate, int cell, int memofs)
    {
        int* pos = (int*)(gCelldata[cell].mDynmem + memofs);
        int* dir = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        double* lastgate = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int)*2);

        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        int reset = gate < 0;
        gate = gate < 0 ? -gate : gate;
        if (gate < 0.5)
        {
            *lastgate = gate;
            return gWavHandle[handle]->data[*pos];
        }
        if (*lastgate < 0.5 && reset)
        {
            *pos = 0;
            *dir = 0;
        }
        *lastgate = gate;

        int t = (int)type;
        if (*dir == 0)
        {
            *dir = 1;
            if (t == 2)
                *dir = -1;
        }
        /*
        0 forward
        1 pingpong
        2 reverse
        */

        if (*pos < 0)
        {
            if (t == 1)
            {
                *dir = 1;
                *pos = 1;
            }
            else
            {
                *pos = gWavHandle[handle]->frames - 1;
            }
        }
        
        if (*pos >= gWavHandle[handle]->frames)
        {
            if (t == 1)
            {
                *pos = gWavHandle[handle]->frames - 2;
                *dir = -1;
            }
            else
            {
                *pos = 0;
            }
        }

        double s = gWavHandle[handle]->data[*pos];
        (*pos) += (*dir);

        return s;
    }

    double playloopp(double v, double loopstart, double loopend, double type, double gate, int cell, int memofs)
    {
        int* pos = (int*)(gCelldata[cell].mDynmem + memofs);
        int* dir = (int*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        double* lastgate = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) * 2);

        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        int reset = gate < 0;
        gate = gate < 0 ? -gate : gate;
        if (gate < 0.5)
        {
            *lastgate = gate;
            return gWavHandle[handle]->data[*pos];
        }
        if (*lastgate < 0.5 && reset)
        {
            *pos = 0;
            *dir = 0;
        }
        *lastgate = gate;

        int t = (int)type;
        if (*dir == 0)
        {
            *dir = 1;
            if (t == 2)
                *dir = -1;
        }
        /*
        0 forward
        1 pingpong
        2 reverse
        */
        int lsp = (int)(loopstart * gSamplerate);
        if (lsp < 0) lsp = 0;
        if (lsp >= gWavHandle[handle]->frames)
            lsp = gWavHandle[handle]->frames-1;
        int lep = (int)(loopend * gSamplerate);
        if (lep < 0) lep = 0;
        if (lep >= gWavHandle[handle]->frames)
            lep = gWavHandle[handle]->frames-1;

        if (lsp > lep)
        {
            int temp = lsp; lsp = lep; lep = temp;
        }

        if (*pos < lsp && *dir == -1)
        {
            if (t == 1)
            {
                *dir = 1;
                *pos = lsp + 1;
            }
            else
            {
                *pos = lep - 1;
            }
        }

        if (*pos >= lep && *dir == 1)
        {
            if (t == 1)
            {
                *pos = lep - 1;
                *dir = -1;
            }
            else
            {
                *pos = lsp;
            }
        }

        if (*pos < lsp) *pos = lsp;
        if (*pos >= lep) *pos = lep;

        double s = gWavHandle[handle]->data[*pos];
        (*pos) += (*dir);

        return s;
    }

    double sample(double v, double pos)
    {
        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        double p = pos * gSamplerate;
        int p1 = (int)p;
        double f1 = p - p1;
        int frames = gWavHandle[handle]->frames;
        double a = gWavHandle[handle]->data[(p1 + frames - 1) % frames];
        double b = gWavHandle[handle]->data[(p1) % frames];
        double c = gWavHandle[handle]->data[(p1 + 1) % frames];
        double d = gWavHandle[handle]->data[(p1 + 2) % frames];

        return catmullrom(f1, a, b, c, d);
    }

    double samplefast(double v, double pos)
    {
        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        int p1 = (int)(pos * gSamplerate);
        int frames = gWavHandle[handle]->frames;

        return gWavHandle[handle]->data[(p1) % frames];
    }


    double playloopx(double v, double loopstart, double loopend, double fade, double gate, int cell, int memofs)
    {
        int* pos = (int*)(gCelldata[cell].mDynmem + memofs);
        double* lastgate = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int));
        double* lastout = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(int) + sizeof(double));

        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        int reset = gate < 0;
        gate = gate < 0 ? -gate : gate;
        if (*lastgate > 0.5)
            reset = 0;
        *lastgate = gate;
        if (gate < 0.5)
            return *lastout;

        int f = (int)(fade * gSamplerate);
        if (f < 1) f = 1;

        int lsp = (int)(loopstart * gSamplerate);
        if (lsp < 0) lsp = 0;
        if (lsp >= gWavHandle[handle]->frames)
            lsp = gWavHandle[handle]->frames - 1;

        int lep = (int)(loopend * gSamplerate);
        if (lep < 0) lep = 0;
        if (lep >= gWavHandle[handle]->frames)
            lep = gWavHandle[handle]->frames - 1;

        if (lsp > lep)
        {
            int temp = lsp; lsp = lep; lep = temp;
        }

        if (*pos >= lep - f)
        {
            *pos = lsp;
        }

        if (*pos < lsp) *pos = lsp;

        double s = gWavHandle[handle]->data[*pos];
        if (*pos < lsp + f)
        {
            int d = *pos - lsp; // d = 0..f
            double t = gWavHandle[handle]->data[lep - f + d];
            s += ((t - s) * (f - d)) / f;
        }
        
        if (reset)
            *pos = lsp;
        else
            (*pos)++;
        
        *lastout = s;
        return s;
    }

    double grain(double v, double pos, double grainsize, double fade, int cell, int memofs)
    {
        int* samplepos = (int*)(gCelldata[cell].mDynmem + memofs);

        unsigned int handle = (int)v - ((int)'S' << 24);

        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return 0;

        int f = (int)(fade * gSamplerate);
        if (f < 1) f = 1;

        int g = (int)(grainsize * gSamplerate);
        if (g <= f) g = f+1;

        int maxlen = gWavHandle[handle]->frames - (1 + g);
        if (maxlen < 1) maxlen = 1;

        int p = (unsigned int)(pos * gSamplerate);
        p = (unsigned int)p % maxlen;

        if (*samplepos < p) *samplepos = p;
        if (*samplepos >= p + g - f) *samplepos = p;


        double s = gWavHandle[handle]->data[*samplepos];
        if (*samplepos < p + f)
        {
            int d = *samplepos - p; // d = 0..f
            double t = gWavHandle[handle]->data[p + g - f + d];
            s += ((t - s) * (f - d)) / f;
        }
        
        (*samplepos)++;
        return s;
    }

    double buffer(double len, int cell, int memofs)
    {
        WavResource** res = (WavResource**)(gCelldata[cell].mDynmem + memofs);
        if (!*res)
        {
            char temp[64];
            sprintf(temp, "buffer\"%x%x%x", (int)(len*1000),cell, memofs);
            *res = (WavResource*)getResource(temp, len, 2);
            if (!*res) return 0;
        }
        if (*res == (WavResource*)-1)
            return 0;
        return (*res)->handle;
    }

    int klatthash(const char* t)
    {
        int hash = 0;
        int x = 0;
        while (*t)
        {
            x ^= *t;
            hash += x;
            t++;
        }
        return hash;
    }

    double klatt(char* txt, int cell, int memofs)
    {
        WavResource** res = (WavResource**)(gCelldata[cell].mDynmem + memofs);

        if (!*res)
        {
            char temp[256];
            sprintf(temp, "kl\"%x%x", cell, memofs);
            *res = (WavResource*)getResource(temp, 0, 3, txt);
            if (*res)
            {
                int hash = klatthash(txt);
                if ((*res)->hash == 0)
                    (*res)->hash = hash;
                if ((*res)->hash != hash)
                    *res = (WavResource*)regetResource(temp, 0, 3, txt);
            }
            if (!*res) return 0;
        }

        if (*res == (WavResource*)-1)
            return 0;

        return (*res)->handle;
    }

    double padsynth(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs)
    {
        WavResource** res = (WavResource**)(gCelldata[cell].mDynmem + memofs);
        double* lastparms = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(void*));
        if (!*res)
        {
            lastparms[0] = bw;
            lastparms[1] = bws;
            lastparms[2] = relf;
            lastparms[3] = a1;
            lastparms[4] = a2;
            lastparms[5] = a3;
            lastparms[6] = a4;
            lastparms[7] = a5;
            lastparms[8] = a6;
            lastparms[9] = a7;
            lastparms[10] = a8;
            char temp[64];
            sprintf(temp, "ps\"%x%x", cell, memofs);
            *res = (WavResource*)getResource(temp, 0, 4, lastparms);
            if (!*res) return 0;
        }
        if (*res == (WavResource*)-1)
            return 0;
        if (lastparms[0] != bw ||
            lastparms[1] != bws ||
            lastparms[2] != relf ||
            lastparms[3] != a1 ||
            lastparms[4] != a2 ||
            lastparms[5] != a3 ||
            lastparms[6] != a4 ||
            lastparms[7] != a5 ||
            lastparms[8] != a6 ||
            lastparms[9] != a7 ||
            lastparms[10] != a8)
        {
            char temp[64];
            sprintf(temp, "ps\"%x%x", cell, memofs);
            lastparms[0] = bw;
            lastparms[1] = bws;
            lastparms[2] = relf;
            lastparms[3] = a1;
            lastparms[4] = a2;
            lastparms[5] = a3;
            lastparms[6] = a4;
            lastparms[7] = a5;
            lastparms[8] = a6;
            lastparms[9] = a7;
            lastparms[10] = a8;
            // Potential hazard.. params point here, but what if here isn't there anymore?                        
            *res = (WavResource*)regetResource(temp, 0, 4, lastparms);
            if (!*res) return 0;
            return 0;
        }
        return (*res)->handle;
    }

    double padsynth22(double bw, double bws, double relf, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, int cell, int memofs)
    {
        WavResource** res = (WavResource**)(gCelldata[cell].mDynmem + memofs);
        double* lastparms = (double*)(gCelldata[cell].mDynmem + memofs + sizeof(void*));
        if (!*res)
        {
            lastparms[0] = bw;
            lastparms[1] = bws;
            lastparms[2] = relf;
            lastparms[3] = a1;
            lastparms[4] = a2;
            lastparms[5] = a3;
            lastparms[6] = a4;
            lastparms[7] = a5;
            lastparms[8] = a6;
            lastparms[9] = a7;
            lastparms[10] = a8;
            char temp[64];
            sprintf(temp, "ps22\"%x%x", cell, memofs);
            *res = (WavResource*)getResource(temp, 0, 5, lastparms);
            if (!*res) return 0;
        }
        if (*res == (WavResource*)-1)
            return 0;
        if (lastparms[0] != bw ||
            lastparms[1] != bws ||
            lastparms[2] != relf ||
            lastparms[3] != a1 ||
            lastparms[4] != a2 ||
            lastparms[5] != a3 ||
            lastparms[6] != a4 ||
            lastparms[7] != a5 ||
            lastparms[8] != a6 ||
            lastparms[9] != a7 ||
            lastparms[10] != a8)
        {
            char temp[64];
            sprintf(temp, "ps22\"%x%x", cell, memofs);
            lastparms[0] = bw;
            lastparms[1] = bws;
            lastparms[2] = relf;
            lastparms[3] = a1;
            lastparms[4] = a2;
            lastparms[5] = a3;
            lastparms[6] = a4;
            lastparms[7] = a5;
            lastparms[8] = a6;
            lastparms[9] = a7;
            lastparms[10] = a8;
            *res = (WavResource*)regetResource(temp, 0, 5, lastparms);
            if (!*res) return 0;
            return 0;
        }
        return (*res)->handle;
    }

    double label(char* txt, int cell, int)
    {
        gCelldata[cell].mLabel = txt;
        return 0;
    }

    double biquad(double v, double a0, double a1, double a2, double b0, double b1, double b2, int cell, int memofs)
    {
        double* val = (double*)(gCelldata[cell].mDynmem + memofs);
        // y[n] = (1/a0) * (
        //  b0 * x[n] + 
        //  b1 * x[n-1] + 
        //  b2 * x[n-2] - 
        //  a1 * y[n-1] - 
        //  a2 * y[n-2]
        // )
        if (!isnormal(v)) v = 0;
        double r =
            1.0 / a0 * (
                b0 * v +
                b1 * val[0] +
                b2 * val[1] -
                a1 * val[2] -
                a2 * val[3]);
        if (!isnormal(r)) r = 0;
        // update x[n-1] and x[n-2]
        val[1] = val[0];
        val[0] = r;
        // update y[n-1] and y[n-1]
        val[3] = val[2];
        val[2] = v;
        return r;
    }

    double write(double v, double pos, double val, double gate)
    {
        unsigned int handle = (int)v - ((int)'S' << 24);
        if (handle >= MAX_HANDLES || !gWavHandle[handle])
            return val;
        if (gate < 0.5)
            return val;
        WavResource* res = (WavResource*)gWavHandle[handle];
        int ofs = (int)(pos * gSamplerate);
        if (!isnormal(val)) val = 0; // avoid buffering NaNs
        res->data[((unsigned int)ofs) % res->frames] = (float)val;
        return val;
    }

    double ay(double tone1, double tone2, double tone3, double noise, double vol1, double vol2, double vol3, double envelope, double envshape, double mixer, double envgate, int cell, int memofs)
    {
        struct ayumi *ay = (struct ayumi*)(gCelldata[cell].mDynmem + memofs);
        if (ay->step == 0)
        {
            ayumi_configure(ay, 0, 2000000, gSamplerate);
            ayumi_set_pan(ay, 0, 0, 0);
            ayumi_set_pan(ay, 1, 0, 0);
            ayumi_set_pan(ay, 2, 0, 0);
        } 
        if (tone1 <= 0.001 || !isnormal(tone1)) tone1 = 0.001;
        if (tone2 <= 0.001 || !isnormal(tone2)) tone2 = 0.001;
        if (tone3 <= 0.001 || !isnormal(tone3)) tone3 = 0.001;
        ayumi_set_tone(ay, 0, (int)((2000000 / 16) / tone1));
        ayumi_set_tone(ay, 1, (int)((2000000 / 16) / tone2));
        ayumi_set_tone(ay, 2, (int)((2000000 / 16) / tone3));
        ayumi_set_noise(ay, (int)(0x1f * noise));
        int imixer = (int)mixer;
        ayumi_set_mixer(ay, 0, !((imixer >> 0) & 1), !((imixer >> 3) & 1), (imixer >> 6) & 1);
        ayumi_set_mixer(ay, 1, !((imixer >> 1) & 1), !((imixer >> 4) & 1), (imixer >> 7) & 1);
        ayumi_set_mixer(ay, 2, !((imixer >> 2) & 1), !((imixer >> 5) & 1), (imixer >> 8) & 1);
        ayumi_set_volume(ay, 0, (int)(0xf * vol1));
        ayumi_set_volume(ay, 1, (int)(0xf * vol2));
        ayumi_set_volume(ay, 2, (int)(0xf * vol3));
        if (envelope <= 0.001 || !isnormal(envelope)) envelope = 0.001;
        ayumi_set_envelope(ay, (int)((2000000 / 16) / envelope));
        if (envgate > 0.5) {
            ayumi_set_envelope_shape(ay, (int)(envshape));
        }
        ayumi_process(ay);
        return ay->left + ay->right;
    }

    double slewlimit(double v, double max, int cell, int memofs)
    {
        double* prev = (double*)(gCelldata[cell].mDynmem + memofs);
        if (!isnormal(v)) v = 0;
        if (!isnormal(max)) return *prev;
        double delta = v - *prev;
        if (delta < 0)
        {
            if (-delta > max) delta = -max;
        }
        else
        {
            if (delta > max) delta = max;
        }
        *prev += delta;
        return *prev;
    }

    double slewlimitud(double v, double maxu, double maxd, int cell, int memofs)
    {
        double* prev = (double*)(gCelldata[cell].mDynmem + memofs);
        if (!isnormal(v)) v = 0;
        if (!isnormal(maxu)) return *prev;
        if (!isnormal(maxd)) return *prev;
        double delta = v - *prev;
        if (delta < 0)
        {
            if (-delta > maxd) delta = -maxd;
        }
        else
        {
            if (delta > maxu) delta = maxu;
        }
        *prev += delta;
        return *prev;
    }

    double akwf(double v, double s)
    {
        if (s < 0) s = -s;
        if (v < 0) v = -v;
        int sample = (int)s;
        int sample2 = sample + 1;
        double sf = sample - s;
        sample %= (akwfdata_size / 1200);
        sample2 %= (akwfdata_size / 1200);
        double pf = 600 * v * gSamplerate / 44100.0;
        int p = (int)pf;
        double f = pf - p;

        if (sf == 0) // exact sample value given
        {
            short* data = (short*)akwfdata;
            data += sample * 600;
            double a = data[(p + 600 - 1) % 600] / 32768.0;
            double b = data[(p) % 600] / 32768.0;
            double c = data[(p + 1) % 600] / 32768.0;
            double d = data[(p + 2) % 600] / 32768.0;

            return catmullrom(f, a, b, c, d);
        }
        

        // need to interpolate two samples..
        short* data = (short*)akwfdata;
        data += sample * 600;
        double a1 = data[(p + 600 - 1) % 600] / 32768.0;
        double b1 = data[(p) % 600] / 32768.0;
        double c1 = data[(p + 1) % 600] / 32768.0;
        double d1 = data[(p + 2) % 600] / 32768.0;

        data = (short*)akwfdata;
        data += sample2 * 600;
        double a2 = data[(p + 600 - 1) % 600] / 32768.0;
        double b2 = data[(p) % 600] / 32768.0;
        double c2 = data[(p + 1) % 600] / 32768.0;
        double d2 = data[(p + 2) % 600] / 32768.0;

        double a = a1 - (a2 - a1) * sf;
        double b = b1 - (b2 - b1) * sf;
        double c = c1 - (c2 - c1) * sf;
        double d = d1 - (d2 - d1) * sf;

        return catmullrom(f, a, b, c, d);
    }

    double bbd(double v, double count, int cell, int memofs)
    {
        return bbdeff(v, count, 20, cell, memofs);
    }

    double bbdeff(double v, double count, double eff, int cell, int memofs)
    {
        double* data = (double*)(gCelldata[cell].mDynmem + memofs);
        if (!isnormal(v)) v = 0;
        int c = (int)count;
        if (c < 8) c = 8;
        if (c > 1022) c = 1022;
        int e = (int)eff;
        if (e < 2) e = 2;
        if (e > 100) e = 100;

        for (int k = 0; k < e; k++)
        {
            data[0] = (data[0] + v) / 2;
            for (int j = 0; j < 2; j++)
                for (int i = j; i < c; i += 2)
                {
                    double p = (data[i] + data[i + 1]) / 2;
                    data[i] = p;
                    data[i + 1] = p;
                }
        }

        return data[c - 1];
    }

    double hexwave(double aReflect, double peaktime, double halfheight, double zerowait, double freq, int cell, int memofs)
    {
        char * first = (char*)(gCelldata[cell].mDynmem + memofs + sizeof(HexWave));
        HexWave * osc = (HexWave*)(gCelldata[cell].mDynmem + memofs);
        int reflect = (int)aReflect;
        if (reflect != 0) reflect = 1;
        if (*first == 0)
        {
            *first = 1;
            hexwave_create(osc, reflect, (float)peaktime, (float)halfheight, (float)zerowait);
        }
        else
        {
            hexwave_change(osc, reflect, (float)peaktime, (float)halfheight, (float)zerowait);
        }
        float out;
        hexwave_generate_samples(&out, 1, osc, (float)(freq / gSamplerate));
        return out;
    }

};