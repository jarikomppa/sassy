/*
    PADsynth implementation as ready-to-use C++ class.
    By: Nasca O. Paul, Tg. Mures, Romania
    This implementation and the algorithm are released under Public Domain
    Feel free to use it into your projects or your products ;-)

    This implementation is tested under GCC/Linux, but it's 
    very easy to port to other compiler/OS. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "PADsynth.h"

#include "FFTRealFixLen.h"
ffft::FFTRealFixLen<18> padfft18;
ffft::FFTRealFixLen<22> padfft22;
//#include "FFTReal.h"
//ffft::FFTReal<float> padfft(1 << 19);
#include <string.h>

void ifft(float* smp, int samplecount)
{
    float* a = new float[samplecount];
    if (samplecount == (1 << 18)) padfft18.do_ifft(smp, a);
    if (samplecount == (1 << 22)) padfft22.do_ifft(smp, a);
    memcpy(smp, a, sizeof(float) * samplecount);
    delete[] a;
}

PADsynth::PADsynth(int aSampleCount, float aSamplerate, int aHarmonicsCount)
{
    mDetf = 0;
    mSampleCount = aSampleCount;
    mSamplerate = aSamplerate;
    mHarmonicsCount = aHarmonicsCount;
    mHarmonics = new float[mHarmonicsCount];
    int i;
    for (i = 0; i < mHarmonicsCount; i++)
        mHarmonics[i] = 0.0f;
    mHarmonics[1] = 1.0f;//default, the first harmonic has the amplitude 1.0

    mFreqAmp = new float[mSampleCount / 2];
};

PADsynth::~PADsynth()
{
    delete[] mHarmonics;
    delete[] mFreqAmp;
};

float PADsynth::relF(int N)
{
    return abs((float)N-1 + (float)(mDetf * (N - 1) * (N - 1)))+1;
};

void PADsynth::setharmonic(int n, float value)
{
    if (n < 1 || n >= mHarmonicsCount) return;
    mHarmonics[n] = value;
};

float PADsynth::getharmonic(int n)
{
    if (n < 1 || n >= mHarmonicsCount)
        return 0.0f;
    return mHarmonics[n];
};

float PADsynth::profile(float fi, float bwi)
{
    float x = fi / bwi;
    x *= x;
    if (x > 14.71280603f)
        return 0.0f; //this avoids computing the e^(-x^2) where it's results are very close to zero
    return (float)exp(-x) / bwi;
};

void PADsynth::synth(float f, float bw, float bwscale, float detf, float* smp)
{
    int i, nh;
    mDetf = detf;

    for (i = 0; i < mSampleCount / 2; i++)
        mFreqAmp[i] = 0.0f; //default, all the frequency amplitudes are zero

    for (nh = 1; nh < mHarmonicsCount; nh++)
    { //for each harmonic
        float bw_Hz;//bandwidth of the current harmonic measured in Hz
        float bwi;
        float fi;
        float relf = relF(nh);
        float rF = f * relf;

        bw_Hz = (float)((pow(2.0f, bw / 1200.0f) - 1.0f) * f * pow(relf, bwscale));

        bwi = (float)(bw_Hz / (2.0f * mSamplerate));
        fi = rF / mSamplerate;
        for (i = 0; i < mSampleCount / 2; i++)
        {   //here you can optimize, by avoiding to compute the profile for the full frequency (usually it's zero or very close to zero)
            float hprofile = profile((i / (float)mSampleCount) - fi, bwi);
            mFreqAmp[i] += hprofile * mHarmonics[nh];
        }
    }

    //Convert the freq_amp array to complex array (real/imaginary) by making the phases random
    for (i = 0; i < mSampleCount / 2; i++)
    {
        float phase = RND() * 2.0f * 3.14159265358979f;
        smp[i * 2 + 0] = (float)(mFreqAmp[i] * cos(phase));
        smp[i * 2 + 1] = (float)(mFreqAmp[i] * sin(phase));
    };

    ifft(smp, mSampleCount);

    //normalize the output
    float max = 0.0;
    for (i = 0; i < mSampleCount; i++)
    {
        float amp = (float)fabs(smp[i]);
        if (amp > max)
        {
            max = amp;
        }
    }
    if (max < 0.0001f) max = 0.0001f;
    for (i = 0; i < mSampleCount; i++)
        smp[i] /= max;

};

float PADsynth::RND()
{
    return (rand() / (RAND_MAX + 1.0f));
};


