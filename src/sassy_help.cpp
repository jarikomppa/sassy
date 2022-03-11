#include "sassy.h"
struct FuncInfo {
	const char* head;
	const char* body;
};

struct FuncGroup {
	const char* head;
	FuncInfo func[64]; // 64 functions per group ought to be enough..
};

FuncGroup test[2] = {
	{"a", {{"a","a"},{"a","a"}},},
	{"a", {{"a","a"},{"a","a"}},},
};


FuncGroup gFuncHelp[] = {
{ "Operators", {
{ "-  -> subtraction", "Subtracts a value from another."},
{ "*  -> multiplication", "Multiplies two values together."},
{ "/  -> division (div by zero = zero)", "Divides a value from another."},
{ "+  -> addition", "Adds two values together." },
{"() -> parentheses", "Parentheses can be used to alter (or enforce) calculation order.\nFor example, (1+2)*3 = 9, while 1+2*3 = 7. "},
{"<  -> less than (returns 0 or 1)",  "Primarily meant for the if() function, the < operator returns 0 for not true and 1 for true. "},
{"=  -> equals within 0.0001 (returns 0 or 1)", "Primarily meant for the if() function, the = operator returns 0 for not true and 1 for true. The values can be 0.0001 apart which is generally a good idea when comparing equality of floating point values. "},
{">  -> greater than (returns 0 or 1)", "Primarily meant for the if() function, the > operator returns 0 for not true and 1 for true. "},
{0,0} },},

{ "Basics", {
{"dt()               -> timestep", "The amount of time, in seconds, per sample. When you need to know the exact time a single sample takes. Useful in creating custom clocks (for FM, for instance): a1+dt()*(freq+freqmodifier) - if placed in cell a1, will create a clock that increases predictably. (See step() for a more sane way to do this)."},
{"if(test,then,else) -> then or else", "The if() function checks if the 'test' value is zero or not. If non-zero, the 'then' value is returned, otherwise the 'else' value is. Note that both sides are still evaluated, so any side effects (like MIDI send or sample output) will still happen."},
{"in()               -> sample", "Monophonic input from an audio capture device. If called multiple times, returns the same sample every time."},
{"in(channel)        -> sample (stereo input)", "Stereo input. State desired channel: 0 for left, 1 for right."},
{"label(\"text\")      -> 0", "Gives label to a cell, shown in the UI widgets. Returns zero, so can be added to formulas."},
{"nop(..)            -> 0", "Any function can be renamed to nop to cause it to perform no operation. The arguments will still be evaluated, so if they have side effects (like sending MIDI data), those effects will still occur."},
{"out(l,r)           -> l (stereo output)", "Stereo output. If called multiple times, only one of the calls yields output. Returns the left channel value. "},
{"out(sample)        -> sample", "Monophonic output. If called multiple times, only one of the calls yields output. Also returns its parameter, so this call can be chained with graph(), for example."},
{"rem(\"text\")        -> 0", "Remark. The contents will be ignored and the function returns zero."},
{"step(v)            -> stepped value", "Simple way to step through waveforms with no glitches:\nout(sin1(step(220+midipitch()*100)\ntime() can be replaced with step(1) in most cases. " },
{"step(v, gate)      -> stepped value", "Gated step function, runs when gate is 1, otherwise 0. Useful if you need to reset waveform. "},
{"time()             -> time in seconds", "Always running time value, in seconds. The heartbeat of the system. Consider using step() instead."},
{"time(gate)         -> time in seconds", "Time value that runs when the 'gate' value is 1. Returns 0 otherwise. "},
{"trigger(v)         -> trigger pulse", "Returns 1 on rising edge of signal, otherwise returns 0. "},
{"trigger(v,mode)    -> trigger pulse", "Returns 1 on detected edge of signal, otherwise returns 0.\nMode 0: rising edge\nMode 1: falling edge\nMode 2: both edges\nOthers: no edges "},
{0,0} },},

{ "Signal processing",
{
{"adsr(a,d,s,r,gate)                         -> ADSR envelope", "Attack/Decay/Sustain/Release envelope.\nAttack is time to reach 1.\nDecay is time to reach sustain after attack.\nSustain is volume level to sustain at.\nRelease is time to reach zero after release.\nGate 0 means note is off. If gate is 1, level starts from current value. If gate is -1, level starts from 0.\nOutput values are within 0 and 1."},
{"allpass(sample,time,feedback)              -> filtered sample", "Buffers samples and returns them after the desired time has passed. Maximum 1 second. Can be used for delay/feedback effects. "},
{"bbd(in,bcount)                             -> filtered sample", "Bit bucket device. Experimental. Very expensive. Bcount is the number of buckets. 20 iterations per cycle."},
{"bbd(in,bcount,iters)                       -> filtered sample", "Bit bucket device. Experimental. Very expensive. Bcount is the number of buckets. Iters is the iterations per cycle."},
{"bitcrunch(sample,bits)                     -> crunched sample", "Reduces the bit width of the sample. Can handle fractional bits too. Don't ask."},
{"biquad(v,a0,a1,a2,b0,b1,b2)                -> sample", "Biquad filter, a basic block for various digital filters. Calculates:\ny[n] = (1 / a0) * (b0 * x[n] +  b1 * x[n-1] +  b2 * x[n-2] -  a1 * y[n-1] -  a2 * y[n-2])"},
{"bpf(in,samplerate,resonance)               -> band pass filtered sample","Biquad resonant band pass filter. Returns filtered sample. 'in' is the incoming sample. 'samplerate' is the filter cutoff rate, in Hz. 'resonance' is the sharpness of the cutoff. "},
{"bpf(in,samplerate,resonance,count)         -> band pass filtered sample","The additional 'count' parameter says how many times the filter should be applied, for steeper filter curve. Default 1. "},
{"comb(sample,time,damp,feedback)            -> filtered sample","Buffers samples and returns them after the desired time has passed. Maximum 1 second. Can be used for delay/feedback effects. "},
{"dc(sample)                                 -> detected dc","Calculates running average of samples over 1s, practically detecting any existing dc. Useful for removing bias from signal. "},
{"delay(sample,time)                         -> delayed sample","Buffers samples and returns them after the desired time has passed. Maximum 1 second. Modulating the time parameter may yield strange results."},
{"distort(sample)                            -> distorted sample","Calculates:\nsign(v) * ( 1 - e ^ -|v|)\ncausing soft clipping on louder end. For higher distortion, amplify incoming sample."},
{"filter(in,samplerate,resonance,type)       -> filtered sample","Biquad resonant filter. Returns filtered sample. 'in' is the incoming sample. 'samplerate' is the filter cutoff rate, in Hz. 'resonance' is the sharpness of the cutoff. (1 is a good value to start with) Type is 0 for low, 1 for high and 2 for band pass. See lpf, bpf and hpf for shorthands to this function."},
{"filter(in,samplerate,resonance,type,count) -> filtered sample","The additional 'count' parameter says how many times the filter should be applied, for steeper filter curve. Default 1. "},
{"hold(sample,time)                          -> held sample","Sample and hold. Records the incoming value and keeps returning it until desired time has passed, at which point the process repeats. If the 'time' parameter changes while holding, the hold may be released earlier (or later) than the original value stated."},
{"hold(sample,time,mode)                     -> held sample","Sample and hold. If mode is 0, works like the 2 parameter version. If mode is 1, interpolates between old and new value. if mode is -1, interpolates to the wrong direction. Other values between 0 and 1 return values between old and new value. "},
{"hpf(in,samplerate,resonance)               -> high pass filtered sample","Biquad resonant high pass filter. Returns filtered sample. 'in' is the incoming sample. 'samplerate' is the filter cutoff rate, in Hz. 'resonance' is the sharpness of the cutoff. "},
{"hpf(in,samplerate,resonance,count)         -> high pass filtered sample","The additional 'count' parameter says how many times the filter should be applied, for steeper filter curve. Default 1. "},
{"latch(sample, gate)                        -> latched sample","Latches the incoming value. Returns the value that was given when the 'gate' value went non-zero, until 'gate' returns to zero."},
{"lpf(in,samplerate,resonance)               -> low pass filtered sample","Biquad resonant low pass filter. Returns filtered sample. 'in' is the incoming sample. 'samplerate' is the filter cutoff rate, in Hz. 'resonance' is the sharpness of the cutoff. "},
{"lpf(in,samplerate,resonance,count)         -> low pass filtered sample","The additional 'count' parameter says how many times the filter should be applied, for steeper filter curve. Default 1. "},
{"reverb(sample,roomsize,damp,freeze)        -> filtered sample","Emulates sound reverbing in a room. Rather heavy. Set freeze to 1 to keep reverberating the current data. The output is full wet signal, mix in some of the dry for more pleasing results. "},
{"rubberband(sample,strength)                -> filtered sample","Calculates v=(v+oldvalue*strength)/(strength+1).\nCauses output value to approach input value with a delay. Larger strength values make approach slower. Useful for forcing signals to become continious."},
{"slewlimit(sample,max)                      -> filtered sample","Digital slew limiter. Max states how much the output signal may change."},
{"slewlimit(sample,maxup,maxdown)            -> filtered sample","Digital slew limiter. Maxup states how much the output signal may change upwards, maxdown states how much signal may change downwards."},
{0,0} },},

{ "Waveforms",
{
{"akwf(step,waveno)        -> sample","Mother of all wavetables. Over 4000 Adventure Kid single cycle waveforms ordered in approximate time space similarity. Cubic interpolation within sample, linear interpolation between samples."},
{"bluenoise()              -> random 0..1","Brown noise (more weight to low octaves) giving random numbers between 0 and 1."},
{"brownnoise()             -> random 0..1","Brown noise (more weight to high octaves) giving random numbers between 0 and 1."},
{"drunkardswalk(step,gate) -> walk (0..1)","A random pattern that, on every step gate is 1, makes a random choice of four options:\n- increase by step amount\n- decrease by step amount\n- remain at current value\n- repeat previous choice "},
{"img(\"filename\",xi,yi)    -> -0.5..0.5 pixel", "Loads an image and walks around it based on the x and y increment parameters, returning the brightness of the pixel in -0.5..0.5 range."},
{"noise()                  -> random 0..1","Pure white noise giving random numbers between 0 and 1."},
{"opl1(val)                -> opl style 1 wave (0..0.5)","An AdLib-style waveform (half wave). "},
{"opl2(val)                -> opl style 2 wave (0..0.5)","An AdLib-style waveform (abs sin wave). "},
{"opl3(val)                -> opl style 3 wave (0..0.5)","An AdLib-style waveform (two quarter waves) "},
{"paranoise(10 args)       -> random 0..1","Custom set of weights for noise octaves. giving random numbers between 0 and 1."},
{"pinknoise()              -> random 0..1","Pink noise (equal weights to octaves) giving random numbers between 0 and 1."},
{"pulse(val, duty)         -> pulse wave (-0.5 / 0.5)","Pulse wave with output of -0.5 or 0.5. 'duty' is a value between 0 and 1 which tells how much of the pulse should be high (or low). This is a mathematically pure wave, and as such may be too 'harsh'. Some low pass filtering is recommended to help with aliasing."},
{"saw(val)                 -> saw wave (-0.5..0.5)","Saw wave with output of -0.5 or 0.5. This is a mathematically pure wave, and as such may be too 'harsh'. Some low pass filtering is recommended to help with aliasing."},
{"sawq(val)                -> saw wave built out of sine waves","Saw wave built out of bunch of sine waves. As such this is heavier to compute, but smoother sounding than the 'raw' saw wave. "},
{"sawq(val,o)              -> saw wave built out of sine waves","Saw wave built out of bunch of sine waves. 'o' states how many sines to use. sawq uses 14. As such this is heavier to compute, but smoother sounding than the 'raw' saw wave. "},
{"sin1(val)                -> sine function with period 0..1, out (-0.5..0.5)","Sine wave with period of 0..1 instead of 0..2*pi for convenience, so this can be dropped in place of other waveforms. The output range is also -0.5..0.5 for the same reason."},
{"square(val)              -> square wave (-0.5 / 0.5)","Square wave with output of -0.5 or 0.5. This is equal to pulse wave with duty of 0.5. This is a mathematically pure wave, and as such may be too 'harsh'. Some low pass filtering is recommended to help with aliasing."},
{"squareq(val)             -> square wave built out of sine waves","Square wave built out of bunch of sine waves. As such this is heavier to compute, but smoother sounding than the 'raw' square wave. "},
{"squareq(val,o)           -> square wave built out of sine waves","Square wave built out of bunch of sine waves. 'o' states how many sines to use. squareq uses 10. As such this is heavier to compute, but smoother sounding than the 'raw' square wave. "},
{"squaresaw(val,f)         -> squaresaw wave (-0.5..0.5)","Is it a saw? Is it a square? It's both! When f is 0, the wave is saw, and when it's 1, it's square, if 2, it's triangle, 3 inverse saw, and then it loops back to square."},
{"squaresaw(val,f,duty)    -> squaresaw wave (-0.5..0.5)","The additional parameter is duty cycle for pulse width modulation."},
{"supersaw(val,s,d)        -> supersaw wave (-x..x)","Three octaves of saw wave with scaling and detuning. val is the incoming value s is scale; how much quieter (or louder) each octave should be. 0.75 is a nice value to start with. d is detuning value. Start from 0."},
{"supersin(val,s,d)        -> supersin wave (-x..x)","Three octaves of sin1 wave with scaling and detuning. val is the incoming value s is scale; how much quieter (or louder) each octave should be. 0.75 is a nice value to start with. d is detuning value. Start from 0."},
{"supersquare(val,s,d)     -> supersquare wave (-x..x)","Three octaves of square wave with scaling and detuning. val is the incoming value s is scale; how much quieter (or louder) each octave should be. 0.75 is a nice value to start with. d is detuning value. Start from 0."},
{"triangle(val)            -> triangle wave (-0.5..0.5)","Triangle wave with output of -0.5 or 0.5. "},
{0,0} },},

{"Sample based operations",
{
{"buffer(length)                                -> handle, else 0","Allocates a buffer of specified length. Length cannot be modulated, and has to be a literal value."},
{"channels(handle)                              -> sample channel count", "Returns the number of channels in the file where sample was loaded, or 0 in error (such as invalid handle)."},
{"grain(handle,pos,grainsize,fadesize           -> sample", "Granular player. Typical grain size is around 0.01 (10ms) and fade size is 0.002 (2ms)."},
{"klatt(\"text\")                                 -> handle, else 0", "Performs klatt speech synthesis and returns the result as a sample buffer. Note that if you edit the text, it will generate a new buffer every time you type a letter, so it might be a good idea to change the cell to text before editing the text."},
{"len(handle)                                   -> sample length in seconds", "Returns the length of the sample in seconds. Returns 0 in error (such as invalid handle)." },
{"loadwav(\"filename\")                           -> handle, else 0","Loads a waveform to a buffer which can be played back with other functions. Loads channel 0 from source file. Supports wav, mp3, flac, ogg. Waveform is resampled to Sassy's configured rate."},
{"loadwav(\"filename\",channel)                   -> handle, else 0","Loads a waveform to a buffer which can be played back with other functions. Loads specified channel from source file (first one being 0). Supports wav, mp3, flac, ogg. Waveform is resampled to Sassy's configured rate."},
{"padsynth(bw,bws,dt,a1,a2,a3,a4,a5,a6,a7,a8)   -> handle, else 0", "Generates a padsynth of length 2^18, or about 6 seconds. If the sound feels too repetitive, switch to the longer version.\nbw = bandwidth\nbws = bandwidth scale\ndt = detune, set to 0 by default.\na1..a8 = strength of harmonics" },
{"padsynth22(bw,bws,dt,a1,a2,a3,a4,a5,a6,a7,a8) -> handle, else 0", "Generates a padsynth of length 2^22, or about 90 seconds. Generation takes quite a while. This sounds pretty much the same as the shorter version, so it's best to find the parameters you want with the shorter version and only then switch to this one.\nbw = bandwidth\nbws = bandwidth scale\ndt = detune, set to 0 by default.\na1..a8 = strength of harmonics" },
{"play(handle,trigger)                          -> sample", "Plays sample once when triggered. Triggering returns play position to the start and plays again."},
{"playloop(handle,start,end,type,gate)          -> sample", "Plays samples between start and end when gate is high. Negative gate resets position to start.\nType 0 = forward loop.\nType 1 = pingpong loop.\nType 2 = backwards loop."},
{"playloop(handle,type,gate)                    -> sample", "Plays samples when gate is high.\nType 0 = forward loop. Negative gate resets position to start.\nType 1 = pingpong loop.\nType 2 = backwards loop."},
{"playloopx(handle,start,end,fade,gate)         -> sample", "Plays samples between start and end when gate is high. Negative gate resets position to start. Fade defines crossfade length. Loop is always forward."},
{"sample(handle,pos)                            -> sample", "Returns sample in specified position in the sample. If specified time is past end of buffer, the position loops. Sampling is performed with cubic interpolation, which can be a bit heavy." },
{"samplefast(handle,pos)                        -> sample", "Returns sample in specified position in the sample. If specified time is past end of buffer, the position loops. Sampling is performed with point sampling, which can be a bit noisy." },
{"write(handle,pos,value,gate)                  -> value", "Writes a sample to a buffer when gate is high. Given position loops. The buffer may be allocated with buffer() or it can be created in any other way (like loaded from a mp3 file). Does not change data on disk, only the memory-bound copy."},
{0,0} }, },

{ "User interface",
{
{"bar(area)       -> 0", "Creates an user interface component showing up to 20 bars, showing values between 0 and 1. Always returns 0."},
{"bar(val)        -> val","Creates an user interface bar component which shows a value between 0 and 1 in a bar form. Returns the input value, so this can be chained."},
{"button()        -> 0 / 1","Creates an user interface button component that, while pressed, returns 1, otherwise returns 0. "},
{"encoder()       -> value", "Creates an user interface encoder component that, when dragged, changes value. There is no upper or lower bound. Optional parameter sets the initial value. Right-clicking returns to zero."},
{"fft(val)        -> val","Creates an user interface fft component which can be used to analyze the waveform. (or, more likely, admired, while nodding sagely) Returns the input value, so this can be chained."},
{"graph(val)      -> val","Creates an user interface graph component (or 'scope') which can be used to analyze the waveform. Also allows changing of sync and time scale of display. Returns the input value, so this can be chained."},
{"pie(area)       -> 0", "Creates an user interface component showing up to 20 slices of pie, their relative amounts. Only positive inputs handled.. Always returns 0."},
{"pie(val)        -> val","Creates an user interface pie component which shows a value between -1 and 1 in a pie chart form. Returns the input value, so this can be chained."},
{"plotxy(x)       -> x","Creates an user interface graph component that draws points in (x,x') coordinates."},
{"plotxy(x,y)     -> x","Creates an user interface graph component that draws points in (x,y) coordinates."},
{"probe(ch,v)     -> v","Sends value to be analyzed in the scope window, channel ch. "},
{"probe(v)        -> v","Sends value to be analyzed in the scope window, channel 0. "},
{"slider()        -> 0..1","Creates an user interface slider component that can be used to produce values between 0 and 1. Optional parameter sets the initial value. "},
{"sliderpot(idx)  -> 0..1","Creates an user interface slider component that can be used to produce values between 0 and 1. Index refers to MIDI pot number, so this slider can be controlled via UI or a MIDI controller. Optional parameter sets the initial value. "},
{"toggle()        -> 0 / 1","Creates an user interface toggle component that, while active, returns 1, otherwise returns 0. Optional parameter sets the initial value. "},
{"togglepot(idx)  -> 0 / 1","Creates an user interface toggle component that, while active, returns 1, otherwise returns 0. Index refers to MIDI pot number, so this slider can be controlled via UI or a MIDI controller. Optional parameter sets the initial value. "},
{0,0} },},

{"MIDI interface",
{
{"midichannel(nn)        -> midi channel for note number","Polyphonic MIDI in. Returns the midi channel the note was received on."},
{"midinote()             -> midi note","Returns most recently received MIDI note value. "},
{"midinote(nn)           -> midi note","Polyphonic MIDI in. Returns MIDI note value for the note. Note numbers are allocated by order of arrival."},
{"midinote(nn,ch)        -> midi note","Polyphonic per channel MIDI in. Return MIDI note value for the note from specific MIDI channel. Note numbers are allocated by order of arrival."},
{"midion()               -> midi note on 0 / 1","Returns 0 or 1 depending on whether a note is being pressed. "},
{"midion(nn)             -> midi note on 0 / 1", "Polyphonic MIDI in. Returns 0 or 1 depending on whether a note is being pressed. Note numbers are allocated by order of arrival."},
{"midion(nn,ch)          -> midi note on 0 / 1", "Polyphonic per channel MIDI in. Returns 0 or 1 depending on whether a note is being pressed. Note numbers are allocated by order of arrival."},
{"midiout(note,vel)      -> note","Sends note on when velocity is above 0.\nSends polyphonic aftertouch if velocity changes.\nSends note off if velocity is 0.\nSends note off if note changes.\nIf parameters stay the same, doesn't send anything.\n\nAll midi out sends depend on rate limiting; if too many midi messages have been sent out, the new ones are simply not sent. Most functions will attempt again later until sent. " },
{"midioutpot(value,pot)  -> value","Sends pot value if changed.\n\nAll midi out sends depend on rate limiting; if too many midi messages have been sent out, the new ones are simply not sent. Most functions will attempt again later until sent. " },
{"midioutprog(prog)      -> prog","Sends program change value if changed.\n\nAll midi out sends depend on rate limiting; if too many midi messages have been sent out, the new ones are simply not sent. Most functions will attempt again later until sent. " },
{"midioutraw(gate,a,b,c) -> gate","Sends raw 3-byte midi message if gate is 1. The values are completely unchecked. Use trigger() to only try sending once, instead of spamming the midi output indefinitely.\n\nAll midi out sends depend on rate limiting; if too many midi messages have been sent out, the new ones are simply not sent. Most functions will attempt again later until sent. " },
{"midipitch()            -> midi pitch bend 0..1","Returns most recently received MIDI pitch bend value. " },
{"midipot(potno)         -> midi pot value","Returns most recently received MIDI pot (or mod wheel) value. To know which pot number to use, use the controller and see what number is reported. Optional parameter sets the initial value. " },
{"midiprog()             -> midi program number","Returns most recently received MIDI program value. "},
{"midival()              -> midi note value (hz)","Returns most recently received MIDI note value converted to frequency. " },
{"midival(nn)            -> midi note value (hz)","Polyphonic MIDI in. Returns note's MIDI note value converted to frequency. Note numbers are allocated by order of arrival." },
{"midival(nn,ch)         -> midi note value (hz)","Polyphonic per channel MIDI in. Returns note's MIDI note value converted to frequency. Note numbers are allocated by order of arrival." },
{"midivel()              -> midi note velocity 0..1","Returns most recently received MIDI note velocity, including aftertouch. " },
{"midivel(nn)            -> midi note velocity 0..1","Polyphonic MIDI in. Returns note's MIDI note velocity, including aftertouch. Note numbers are allocated by order of arrival." },
{"midivel(nn,ch)         -> midi note velocity 0..1","Polyphonic per channel MIDI in. Returns note's MIDI note velocity, including aftertouch. Note numbers are allocated by order of arrival." },
{0,0} }, },

{ "Math",
{
{"abs(val)              -> absolute of val","abs(-1) = 1. abs(1) = 1. abs(nan) = nan."},
{"catmullrom(v,a,b,c,d) -> point in catmull-rom spline", "Calculates Catmull-Rom spline where v is 0..1 point in spline and a,b,c,d are control points. Useful for compression, wave shaping, or otherwise making thing all smooth like. Catmull-Rom has the special property that 0 is always b, and 1 is always c."},
{"clamp(v,a,b)          -> clamps v between a and b","a is assumed to be smaller than b."},
{"even(val)             -> 1 if integer of val is divisible by 2",".."},
{"exp(val)              -> exponential function of val",".."},
{"floor(val)            -> val rounded down","floor(0.7) = 0. floor(1.3) = 1."},
{"fract(val)            -> fractional part of val","fract(0.7) = 0.7, fract(1.3) = 0.3"},
{"freqtonote(val)       -> note value of frequency","Turns Hz into midi note values. This calculates 12 * log(32 * pow(2, 3/4.0) * (freq/440 / log(2) "},
{"isnan(val)            -> 0 or 1","Checks if the value is nan, and returns 1 if so."},
{"log(val)              -> natural logarithm of val",".."},
{"log10(val)            -> base-10 logarithm of val",".."},
{"map(v,a,b,c,d)        -> maps v from a..b to c..d","a is assumed to be smaller than b, and c smaller than d. If v is ouside a..b range, the results are extrapolated. "},
{"max(a,b)              -> larger of a or b",".."},
{"min(a,b)              -> smaller of a or b",".."},
{"mix(val,a,b)          -> mix of a and b based on val","If val is 0, returns a. If val is 1, returns b. Interpolates between a and b. If val is outside 0..1 range, the result is extrapolated. "},
{"mod(val, divisor)     -> modulo of val","Like the clock. mod(20,12) = 8pm."},
{"nan()                 -> nan","Returns nan. Which is not a number. Or error state."},
{"nankill(val)          -> val or 0 if val was nan","Cleans up nans in samples. May be useful sometimes."},
{"notetofreq(val)       -> frequency of note","Turns midi notes into Hz. This calculates pow(2, (note - 69) / 12) * 440 but only on integer note values, given that 'pow' is a very expensive operation on intel architectures."},
{"notetofreqslow(val)   -> frequency of note","Turns midi notes into Hz. This calculates pow(2, (note - 69) / 12) * 440 using 'pow', so it handles fractional values. 'pow' is a very expensive operation on intel architectures."},
{"odd(val)              -> 0 if integer of val is divisible by 2",".."},
{"pi()                  -> 3.14...","Returns the constant Pi. Useful in trigonometry."},
{"pow(x,y)              -> x to the power of y","pow(2,4) = 2 * 2 * 2 * 2. Note that this is a surprisingly expensive operation, at least on intel architectures. "},
{"quantize(val)         -> quantize hz to note value","Basically same as turning Hz into note values, getting the integer value, and then going back."},
{"scale(note,scale)     -> note in specified scale", "Takes in a MIDI note number, offset at middle c (60) and converts it to specified scale.\n\n0: Aeolian mode\n1: Algerian scale\n2: Altered scale\n3: Augmented scale\n4: A melodic minor scale\n5: Bebop dominant scale\n6: Blues scale\n7: Dorian mode\n8: Enigmatic scale\n9: Flamenco mode\n10: Half diminished scale\n11: Harmonic major scale\n12: Harmonic minor scale\n13: Hirajoshi scale\n14: Hungarian gypsy scale\n15: Hungarian major scale\n16: Insen scale\n17: Ionian mode\n18: Iwato scale\n19: Locrian mode\n20: Lydian augmented scale\n21: Lydian dominant scale\n22: Lydian mode\n23: Major bebop scale\n24: Major locrian scale\n25: Mixolydian mode\n26: Miyako-bushi scale\n27: Neapolitan major scale\n28: Neapolitan minor scale\n29: Octatonic scale\n30: Pentatonic major scale\n31: Pentatonic minor scale\n32: Persian scale\n33: Phrygian dominant scale\n34: Phrygian mode\n35: Prometheus scale\n36: Tritone scale\n37: Two-semitone tritone scale\n38: Ukrainian Dorian mode"},
{"sign(val)             -> -1 if val is < 0, otherwise 1",".."},
{"smootherstep(val)     -> smootherstepped value","Assumes val is within 0..1."},
{"smoothstep(val)       -> smoothstepped value","Assumes val is within 0..1."},
{"sqrt(val)             -> square root of val",".."},
{"trunc(val)            -> integer part of val","trunc(0.7) = 0. trunc(1.3) = 1."},
{0,0} }, },

{"Trigonometry",
{
{"acos(val)     -> arc-cos of val",".."},
{"asin(val)     -> arc-sin of val",".."},
{"atan(val)     -> arc-tan of val",".."},
{"atan2(x,y)    -> arc-tan of x/y",".."},
{"cos(val)      -> cosine of val",".."},
{"cosh(val)     -> hyperbolic cos of val",".."},
{"degrees(val)  -> radians to degrees",".."},
{"radians(val)  -> degrees to radians",".."},
{"sin(val)      -> sine function with period 0..2*pi",".."},
{"sinh(val)     -> hyperbolic sin of val",".."},
{"tan(val)      -> tan of val",".."},
{"tanh(val)     -> hyperbolic tan of val",".."},
{0,0} }, },

{"Table functions",
{
{"average(area)        -> Average value in area","Scans area for numeric values and returns the average of the values found."},
{"columnof(cell)       -> Numeric index of cell","This function returns the numeric column index of a cell. This can be useful in hilighting or replacing cell content. "},
{"count(area)          -> Number of values in area","Scans area for numeric values and returns the number of values found."},
{"find(area, v)        -> Index of value in area","Scans area for the desired value and returns the index of the first one found. Returns nan if not found. This is the counterpart to select(). "},
{"findv(area, v)       -> Index of value in area","Scans area for the desired value and returns the index of the first one found. Returns nan if not found. This is the counterpart to selectv()."},
{"hilight(row,col)     -> hilight specified cell","Optionally add third parameter for color: hilight(a,b,rgb(1,0.5,0.25)) "},
{"lookup(row,col)      -> retrieve value of cell",".."},
{"max(area)            -> Maximum value in area","Scans area for numeric values and returns the biggest one found."},
{"min(area)            -> Minimum value in area","Scans area for numeric values and returns the smallest one found."},
{"product(area)        -> Product of values in area","Scans area for numeric values and returns the product of the values found. I.e, multiplies them together."},
{"replace(row,col,val) -> replace value in specified cell","This function is potentially dangerous. It *will* replace whatever is in target cell. Only use it if you know what you're doing. For a bit of safety, use rowof() and columnof(). "},
{"rowof(cell)          -> Numeric index of cell","This function returns the numeric row index of a cell. This can be useful in hilighting or replacing cell content. "},
{"select(area,v)       -> Selected value in area","Scans area for numeric values in typewriter order (left to right row by row, top down) and returns the Nth value, where N is integer part of v modulo the number of cells in the area. Which sounds more complicated than it is. Handy for sequencing. "},
{"selectv(area,v)      -> Selected value in area","Scans area for numeric values in column order (top down column by column, left to right) and returns the Nth value, where N is integer part of v modulo the number of cells in the area. Which sounds more complicated than it is. Handy for sequencing. "},
{"sum(area)            -> Sum of values in area","Scans area for numeric values and returns the sum of the values found."},
{0,0} }, },

{"Boolean operations",
{
{"and(a,b) -> 1 if both are 1, else 0","Technically, over 0.0001 is considered 1."},
{"false()  -> 0",".."},
{"not(a)   -> 1 a is 0, else 0","Technically, over 0.0001 is considered 1."},
{"or(a,b)  -> 1 if either are 1, else 0","Technically, over 0.0001 is considered 1."},
{"true()   -> 1",".."},
{"xor(a,b) -> 1 if one is 1, else 0","Technically, over 0.0001 is considered 1."},
{0,0} }, },

{"Binary operations",
{
{"b_and(a,b)             -> a & b","Calculates bitwise and of a and b. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_clear(v,bit)         -> value with bit cleared","Clear indexed bit in value. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_nand(a,b)            -> ~(a & b)","Calculates bitwise nand of a and b. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_nor(a,b)             -> ~(a | b)","Calculates bitwise nor of a and b. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_not(a)               -> ~a","Negates bits in a. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_or(a,b)              -> a | b","Calculates bitwise or of a and b. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_rotleft(v,amt,bits)  -> (v << amt) | (v >> (bits-amt)","Rotates bits to the left. Overflowing bits return from the left. Third parameter is optional; default is 32 The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_rotright(v,amt,bits) -> (v << amt) | (v >> (bits-amt)","Rotates bits to the right. Underflowing bits return from the left. Third parameter is optional; default is 32 The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_set(v,bit)           -> value with bit set","Set indexed bit in value. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_shleft(v,amt)        -> v << amt","Shifts bits to the left. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_shright(v,amt)       -> v >> amt","Shifts bits to the right. Negative values are not bit-extended. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_test(v,bit)          -> 1 if bit is on","Check if indexed bit is on. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{"b_xor(a,b)             -> a ^ b","Calculates bitwise xor of a and b. The b_functions are binary ops and work on integer values. Any fractions are discarded. "},
{0,0} }, },

{"Synth",
{
{"ay(t1,t2,t3,noise,v1,v2,v3,env,envshape,mixer,envgate) -> sample", "AY-3-8910 emulation.\n- t1,t2,t3 control the tone of the three square wave generators. Value is in Hz, conversion to period is done internally.\n- noise is the pitch of the noise. Values 0-1. Conversion to 5 bits done internally.\n- v1,v2,v3 control the volume of the three channels. Values 0-1. Conversion to 4 bits done internally.\n- env is envelope period, in Hz. Coversion to period is done internally.\n- Envshape select one of the 16 envelope shapes. Useful values are 0,4,8,10,11,12,13 and 14. Others are duplicates. Envelope shape is set and envelope is reset when envgate is high.\n- Mixer enables tones, noises and envelopes on channels:\n  1,  2,  4 - tone enable\n  8, 16, 32 - noise enable\n 64,128,256 - envelope enable\nCombine values for different mixer settings." },
{"hexwave(reflect,peaktime,halfheight,zerowait,freq)     -> sample", "stb_hexwave synth\nReflect is either 0 or 1. Other values generally scale from 0 to 1. Or maybe -1. Feel free to experiment."},
{"sidfilter(sample,mode,res,freq)                        -> filtered sample", "SID-inspired filter.\n- Mode:\n  1 - highpass\n  2 - bandpass\n  4 - lowpass\n  8 - 8580 (defaults to 6581)\n  All mode values can be combined.\n- res is resonance, integer values. SID hardware goes to 15, we can go higher.\n- freq is cutoff frequency, integer values. SID hardware goes to 2048, we can go higher."},
{"sidvoice(freq,pulseduty,mode)                          -> sample", "SID-inspired voice\n- freq is frequency. Converted to integer values internally.\n- pulseduty is the pulse wave on/off duty. 0..1, converted to integer values internally.\n- Mode:\n  1 - triangle\n  2 - saw\n  4 - pulse\n  8 - noise\n 16 - \"test\"\n 32 - 6581 (8580 default)\n  All mode values can be combined. If more than one waveform is enabled the results are.. odd, but that's SID for you."},
{"sidenvelope(attack,decay,sustain,release,gate)         -> 0..1", "SID-inspired envelope\nAll values 0..1, and converted to 4 bits internally."},
{0,0} }, },

{ 0, { {0,0} }, } };


void do_show_help_window()
{
	ImGui::SetNextWindowSize(ImVec2(500 * gConfig.mUIScale, 640 * gConfig.mUIScale), ImGuiCond_Appearing);
	ImGui::Begin("Function Help", &gShowHelpWindow);// , ImGuiWindowFlags_AlwaysAutoResize);
	int i = 0, j = 0;
	while (gFuncHelp[i].head)
	{
		if (ImGui::CollapsingHeader(gFuncHelp[i].head))
		{
			j = 0;
			while (gFuncHelp[i].func[j].head)
			{
				if (ImGui::TreeNode(gFuncHelp[i].func[j].head))
				{
					ImGui::TextWrapped(gFuncHelp[i].func[j].body);
					ImGui::TreePop();
					ImGui::Separator();
				}
				j++;
			}
		}
		i++;
	}
	ImGui::End();
}
