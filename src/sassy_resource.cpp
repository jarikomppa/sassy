#include "sassy.h"
#include <string>
#include <map>
#include <vector>
#include "stb/stb_image.h"
#define DR_WAV_IMPLEMENTATION
#include "dr/dr_wav.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr/dr_flac.h"
#define DR_MP3_IMPLEMENTATION
#include "dr/dr_mp3.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"
#include "libsamplerate/samplerate.h"
#include "klatt/darray.h"
#include "klatt/resonator.h"
#include "klatt/klatt.h"
#include "klatt/tts.h"
#include "padsynth/PADsynth.h"

struct ResourceRequest
{
	int type = 0;
	char* fn = 0;
	double parm = 0;
	void* parms = 0;
	~ResourceRequest()
	{
		delete[] fn;
	}
};

std::vector<ResourceRequest*> gResourceRequests;
std::map<std::string, Resource*> gResources;


float* loadwav(char* fn, int& aFrames, int& aChannels, int aChannel)
{
	char filename[1024];
	sprintf(filename, "%s", fn);
	FILE* f = fopen(filename, "rb");
	if (!f)
	{
		sprintf(filename, "%s", gConfig.mLastFilename);
		int slashpos = (int)strlen(filename);
		while (slashpos && filename[slashpos] != '\\' && filename[slashpos] != '/') slashpos--;
		sprintf(filename + slashpos + 1, "%s", fn);
		f = fopen(filename, "rb");
		if (!f)
			return 0;
	}
	fclose(f);
	// First let's try .wav
	unsigned int channels;
	unsigned int samplerate;
	drwav_uint64 totalPCMFrameCount;
	float* pSampleData = 0;
	float* data = 0;
	pSampleData = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, &samplerate, &totalPCMFrameCount, NULL);
	if (pSampleData) {
		aFrames = (int)totalPCMFrameCount;
		aChannels = channels;
		aChannel %= aChannels;
		data = new float[aFrames];
		for (int i = 0; i < aFrames; i++)
			data[i] = pSampleData[i * aChannels + aChannel];
		drwav_free(pSampleData, NULL);
	}
	else
	{
		// Maybe it's mp3?
		drmp3_config mp3conf;
		pSampleData = drmp3_open_file_and_read_pcm_frames_f32(filename, &mp3conf, &totalPCMFrameCount, NULL);
		if (pSampleData) {
			aFrames = (int)totalPCMFrameCount;
			aChannels = mp3conf.channels;
			aChannel %= aChannels;
			samplerate = mp3conf.sampleRate;
			data = new float[aFrames];
			for (int i = 0; i < aFrames; i++)
				data[i] = pSampleData[i * aChannels + aChannel];
			drmp3_free(pSampleData, NULL);
		}
		else
		{
			// ..flac..?
			pSampleData = drflac_open_file_and_read_pcm_frames_f32(filename, &channels, &samplerate, &totalPCMFrameCount, NULL);
			if (pSampleData) {
				aFrames = (int)totalPCMFrameCount;
				aChannels = channels;
				aChannel %= aChannels;
				data = new float[aFrames];
				for (int i = 0; i < aFrames; i++)
					data[i] = pSampleData[i * aChannels + aChannel];
				drflac_free(pSampleData, NULL);
			}
			else
			{
				// ..vorbis..?!
				signed short* s16samples;
				totalPCMFrameCount = stb_vorbis_decode_filename(filename, (int*)&channels, (int*)&samplerate, &s16samples);
				if (totalPCMFrameCount != -1)
				{
					// TODO: use more complex stb_vorbis API to get float samples directly
					aFrames = (int)totalPCMFrameCount;
					aChannels = channels;
					aChannel %= aChannels;
					data = new float[aFrames];
					for (int i = 0; i < aFrames; i++)
						data[i] = s16samples[i * aChannels + aChannel] * (1.0f / 0x8000);
					free(s16samples);
				}
			}
		}
	}

	if (data)
	{
		for (int i = 0; i < aFrames; i++)
		{
			if (data[i] < -1) data[i] = -1;
			if (data[i] > 1) data[i] = 1;
		}
	}

	if (data && (int)samplerate != gSamplerate)
	{
		// Need resampling
		SRC_DATA sd;
		sd.data_in = data;
		sd.input_frames = (long)totalPCMFrameCount;

		int newsamplecount = (int)((double)totalPCMFrameCount * ((double)gSamplerate / (double)samplerate));
		sd.data_out = new float[newsamplecount];
		sd.output_frames = newsamplecount;
		sd.src_ratio = (double)gSamplerate / (double)samplerate;
		int res = src_simple(&sd, gConfig.mResourceResampler, 1);
		delete[] data;
		data = sd.data_out;
		aFrames = newsamplecount;
		if (res)
		{
			delete[] data;
			data = 0;
		}
	}
	return data;
}

float* genspeech(const char* txt, int& aFrames)
{
	int samplerate = 11025;
	int totalPCMFrameCount = 0;
	float* data;

	darray element;
	darray phone;
	xlate_string(txt, &phone);
	int frames = klatt::phone_to_elm(phone.getData(), phone.getSize(), &element);

	klatt synth;
	synth.init();
	totalPCMFrameCount = synth.mNspFr * frames;
	data = new float[totalPCMFrameCount];
	short *sample = new short[synth.mNspFr * 100];
	synth.initsynth(element.getSize(), (unsigned char*)element.getData());	
	int samplecount = synth.synth(0, sample); // ignores the first param..
	int dst = 0;
	while (samplecount > 0)
	{
		for (int i = 0; i < samplecount; i++)
			data[dst++] = sample[i] * (1 / (float)0x8000);
		samplecount = synth.synth(0, sample);
	}
	delete[] sample;

	if (data && (int)samplerate != gSamplerate)
	{
		// Need resampling
		SRC_DATA sd;
		sd.data_in = data;
		sd.input_frames = (long)totalPCMFrameCount;

		int newsamplecount = (int)((double)totalPCMFrameCount * ((double)gSamplerate / (double)samplerate));
		sd.data_out = new float[newsamplecount];
		sd.output_frames = newsamplecount;
		sd.src_ratio = (double)gSamplerate / (double)samplerate;
		int res = src_simple(&sd, gConfig.mResourceResampler, 1);
		delete[] data;
		data = sd.data_out;
		aFrames = newsamplecount;
		if (res)
		{
			delete[] data;
			data = 0;
		}
	}
	return data;
}


float* genpad(double* parms, int& frames, int pot)
{
	frames = (1 << pot);
	float* data = new float[frames];
	PADsynth syn(frames, (float)gSamplerate, 9);
	for (int i = 0; i < 8; i++)
	{
		syn.setharmonic(i + 1, (float)parms[i+3]);
	}
	syn.synth(220, (float)parms[0], (float)parms[1], (float)parms[2], data);
	return data;
}


void checkResourceRequests()
{
	if (gResourceRequests.size() && gResourceLoaderRunning == 0)
	{
		SDL_LockMutex(gAudioMutex);
		gResourceLoaderRunning = 1;
		SDL_DetachThread(SDL_CreateThread(fullfillResourceRequests, "ResouceLoaderThread", 0));
		SDL_UnlockMutex(gAudioMutex);
	}
}

extern volatile bool shutdown;

int fullfillResourceRequests(void*)
{
	while (!shutdown)
	{
		SDL_LockMutex(gAudioMutex);
		if (gResourceRequests.size() == 0)
		{
			gResourceLoaderRunning = 0;
			SDL_UnlockMutex(gAudioMutex);
			return 0;
		}
		// still locked
		char* fn = gResourceRequests.back()->fn;
		gResourceRequests.back()->fn = 0;
		int type = gResourceRequests.back()->type;
		double parm = gResourceRequests.back()->parm;
		double* parms = (double*)gResourceRequests.back()->parms;
		const char* parmc = (const char*)gResourceRequests.back()->parms;
		delete gResourceRequests.back();
		gResourceRequests.pop_back();
		SDL_UnlockMutex(gAudioMutex);

		switch (type)
		{
		case 0: // wav resource
			{
				int chs, fr;
				float* d = loadwav(fn, fr, chs, (int)parm);
				if (d)
				{
					WavResource* r = new WavResource;
					r->fn = fn;
					r->channels = chs;
					r->frames = fr;
					r->data = d;
					r->handle = 0;
					while (gWavHandle[r->handle]) r->handle++;
					gWavHandle[r->handle] = r;
					r->handle |= ((int)'S' << 24);
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = r;
					SDL_UnlockMutex(gAudioMutex);
				}
				else
				{
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = (Resource*)-1;
					SDL_UnlockMutex(gAudioMutex);
					delete[] fn;
				}
			}
			break;
		case 1: // img resource
			{
				char filename[1024];
				sprintf(filename, "%s", fn);
				FILE* f = fopen(filename, "rb");
				if (!f)
				{
					sprintf(filename, "%s", gConfig.mLastFilename);
					int slashpos = (int)strlen(filename);
					while (slashpos && filename[slashpos] != '\\' && filename[slashpos] != '/') slashpos--;
					sprintf(filename + slashpos + 1, "%s", fn);
				}
				if (f) fclose(f);

				int x, y, c;
				stbi_uc* imgdata = stbi_load(filename, &x, &y, &c, 4);
				if (imgdata)
				{
					ImgResource* r = new ImgResource;
					r->fn = fn;
					for (int i = 0; i < 256; i++)
					{
						for (int j = 0; j < 256; j++)
						{
							unsigned int pix = ((unsigned int*)imgdata)[(i * y / 256) * x + (j * x / 256)];
							r->imgdata[i * 256 + j] = (((pix >> 0) & 0xff) + ((pix >> 8) & 0xff) + ((pix >> 16) & 0xff)) / 3;
						}
					}
					stbi_image_free(imgdata);
					r->glhandle = 0xcccccccc; 
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = r;
					SDL_UnlockMutex(gAudioMutex);
				}
				else
				{
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = (Resource*)-1;
					SDL_UnlockMutex(gAudioMutex);
					delete[] fn;
				}
			}
			break;
		case 2: // buffer resource
			{
				WavResource* r = new WavResource;
				r->fn = fn;
				r->channels = 0;
				r->frames = (int)(parm * gSamplerate);
				r->data = new float[r->frames];
				memset(r->data, 0, sizeof(float) * r->frames);
				r->handle = 0;
				while (gWavHandle[r->handle]) r->handle++;
				gWavHandle[r->handle] = r;
				r->handle |= ((int)'S' << 24);
				SDL_LockMutex(gAudioMutex);
				gResources[fn] = r;
				SDL_UnlockMutex(gAudioMutex);
			}
			break;
		case 3: // klatt resource
			{
				int fr = 0;
				float* d = genspeech(parmc, fr);
				if (d)
				{
					WavResource* r = new WavResource;
					r->fn = fn;
					r->channels = 0;
					r->frames = fr;
					r->data = d;
					r->handle = 0;
					while (gWavHandle[r->handle]) r->handle++;
					gWavHandle[r->handle] = r;
					r->handle |= ((int)'S' << 24);
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = r;
					SDL_UnlockMutex(gAudioMutex);
				}
				else
				{
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = (Resource*)-1;
					SDL_UnlockMutex(gAudioMutex);
					delete[] fn;
				}
			}
			break;
		case 4: // padsynth resource
			{
				int fr = 0;
				float* d = genpad(parms, fr, 18);
				if (d)
				{
					WavResource* r = new WavResource;
					r->fn = fn;
					r->channels = 0;
					r->frames = fr;
					r->data = d;
					r->handle = 0;
					while (gWavHandle[r->handle]) r->handle++;
					gWavHandle[r->handle] = r;
					r->handle |= ((int)'S' << 24);
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = r;
					SDL_UnlockMutex(gAudioMutex);
				}
				else
				{
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = (Resource*)-1;
					SDL_UnlockMutex(gAudioMutex);
					delete[] fn;
				}
			}
			break;
		case 5: // padsynth22 resource
			{
				int fr = 0;
				float* d = genpad(parms, fr, 22);
				if (d)
				{
					WavResource* r = new WavResource;
					r->fn = fn;
					r->channels = 0;
					r->frames = fr;
					r->data = d;
					r->handle = 0;
					while (gWavHandle[r->handle]) r->handle++;
					gWavHandle[r->handle] = r;
					r->handle |= ((int)'S' << 24);
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = r;
					SDL_UnlockMutex(gAudioMutex);
				}
				else
				{
					SDL_LockMutex(gAudioMutex);
					gResources[fn] = (Resource*)-1;
					SDL_UnlockMutex(gAudioMutex);
					delete[] fn;
				}
			}
			break;
		}
	}
	gResourceLoaderRunning = 0;
	return 0;
}

void nukeResources()
{
	for (int i = 0; i < MAX_HANDLES; i++)
		gWavHandle[i] = 0;
	for (auto const& x : gResources)
	{
		delete x.second;
	}
	gResources.clear();
	for (auto const& x : gResourceRequests)
	{
		delete x;
	}
	gResourceRequests.clear();
}

Resource* getResource(char* fn, double parm, int type, void* parms)
{
	if (gResources.count(fn) == 0)
	{
		ResourceRequest *r = new ResourceRequest;
		r->type = type;
		r->fn = mystrdup(fn);
		r->parm = parm;
		r->parms = parms;
		gResourceRequests.push_back(r);
	}
	return gResources[fn]; // creates dummy resource
}

Resource* regetResource(char* fn, double parm, int type, void* parms)
{
	if (gResources.count(fn) != 0)
	{
		Resource* r = gResources[fn];
		for (int i = 0; i < MAX_HANDLES; i++)
			if (gWavHandle[i] == r)
				gWavHandle[i] = 0;
		delete r;
		gResources.erase(fn);
	}
	return getResource(fn, parm, type, parms);
}
