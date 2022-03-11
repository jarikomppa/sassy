/*
    PADsynth implementation as ready-to-use C++ class.
    By: Nasca O. Paul, Tg. Mures, Romania
    This implementation and the algorithm are released under Public Domain
    Feel free to use it into your projects or your products ;-)

    This implementation is tested under GCC/Linux, but it's 
    very easy to port to other compiler/OS.

    Please notice that you have to implement the virtual method IFFT() with your IFFT routine.
    
*/

#ifndef PADSYNTH_H
#define PADSYNTH_H


class PADsynth {
public:
	/*  PADsynth:
		N                - is the samplesize (eg: 262144)
		samplerate 	 - samplerate (eg. 44100)
		number_harmonics - the number of harmonics that are computed */
	PADsynth(int aSampleCount, float aSamplerate, int aHarmonicsCount);

	virtual ~PADsynth();

	/* set the amplitude of the n'th harmonic */
	void setharmonic(int n, float value);

	/* get the amplitude of the n'th harmonic */
	float getharmonic(int n);

	/*  synth() generates the wavetable
		f		- the fundamental frequency (eg. 440 Hz)
		bw		- bandwidth in cents of the fundamental frequency (eg. 25 cents)
		bwscale	- how the bandwidth increase on the higher harmonics (recomanded value: 1.0)
		*smp	- a pointer to allocated memory that can hold N samples */
	void synth(float aFundamental, float aBandwidth, float aBandwidthScale, float detf, float* aBuf);
protected:

	/* relF():
		This method returns the N'th overtone's position relative
		to the fundamental frequency.
		By default it returns N.
		You may override it to make metallic sounds or other
		instruments where the overtones are not harmonic.  */
	float relF(int N);

	/* profile():
		This is the profile of one harmonic
		In this case is a Gaussian distribution (e^(-x^2))
			The amplitude is divided by the bandwidth to ensure that the harmonic
		keeps the same amplitude regardless of the bandwidth */
	float profile(float fi, float bwi);

	/* RND() - a random number generator that
		returns values between 0 and 1
	*/
	virtual float RND();

private:
	PADsynth(const PADsynth&); // disable copy
	PADsynth& operator=(PADsynth const&);
	float* mHarmonics;		//Amplitude of the harmonics
	float* mFreqAmp;	//Amplitude spectrum
	float mSamplerate;
	int mHarmonicsCount;
	int mSampleCount;			//Size of the sample
	float mDetf;
};


#endif

