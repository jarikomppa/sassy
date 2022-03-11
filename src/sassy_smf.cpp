#include "sassy.h"
#include "tinyfiledialogs.h"

using namespace EvalFunc;

#define SWAPDWORD(a) ((((a) & 0x000000ff) << 24) | \
                      (((a) & 0x0000ff00) << 8 ) | \
                      (((a) & 0x00ff0000) >> 8 ) | \
                      (((a) & 0xff000000) >> 24))

#define SWAPWORD(a) ((((a) & 0xff00) >> 8) | (((a) & 0xff) << 8))

struct MidiEvent
{
	double time;
	unsigned char d0, d1, d2;
};

std::vector<MidiEvent> gMidiData;

// Read variable-length integer from stream
static int readvar(FILE* f)
{
	int d;
	d = getc(f);
	if (d & 0x80)
	{
		d &= 0x7f;
		int v;
		do
		{
			v = getc(f);
			d = (d << 7) + (v & 0x7f);
		} while (v & 0x80);
	}
	return d;
}

// Read doubleword from stream
static int readdword(FILE* f)
{
	int d;
	fread(&d, 4, 1, f);
	d = SWAPDWORD(d);
	return d;
}

// Read word from stream
static int readword(FILE* f)
{
	short int d;
	fread(&d, 2, 1, f);
	d = SWAPWORD(d);
	return d;
}

// Load chunk header
static int loadchunkheader(FILE* f, int& length)
{
	int id;
	id = readdword(f);
	length = readdword(f);
	return id;
}

// Parse MIDI file
static int parsemidi(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f) return -1;
	int len;
	int id = loadchunkheader(f, len);
	//printf("%08x %d\n", id, len);
	if (id != 'MThd')
	{
		//printf("Bad header id\n");
		fclose(f);
		return -1;
	}
	if (len < 6)
	{
		//printf("Bad header block length\n");
		fclose(f);
		return -1;
	}
	int format = readword(f);
	//printf("format %d\n", format);
	if (format != 1 && format != 0)
	{
		printf("midi loader: Unsupported format\n");
		fclose(f);
		return -1;
	}
	int tracks = readword(f);
	//printf("tracks %d\n", tracks);
	int ppqn = readword(f);
	//printf("ppqn %d\n", ppqn); // pulses (clocks) per quater note
	if (ppqn <= 0)
	{
		printf("midi loader: negative ppqn formats not supported\n");
		fclose(f);
		return -1;
	}
	if (len > 6)
	{
		while (len > 6)
		{
			fgetc(f);
			len--;
		}
	}

	int uspertick = (500000 / ppqn);
	while (!feof(f) && tracks)
	{
		id = loadchunkheader(f, len);
		if (id != 'MTrk')
		{
			printf("Midi loader: Unknown chunk\n");
			fclose(f);
			return -1;
		}
		//printf("\nNew track, length %d\n", len);
		int trackend = 0;
		int command = 0;
		int time = 0;
		while (!trackend)
		{
			int dtime = readvar(f);
			time += dtime;
			//printf("%3.3f ", ((float)time * (float)uspertick) / 1000000.0f);
			int data1 = fgetc(f);
			if (data1 == 0xff)
			{
				data1 = fgetc(f); // sub-command
				len = readvar(f);
				switch (data1)
				{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
					switch (data1)
					{
					case 1:
						//printf("Text:\"");
						break;
					case 2:
						//printf("Copyright:\"");
						break;
					case 3:
						//printf("Track name:\"");
						break;
					case 4:
						//printf("Instrument:\"");
						break;
					case 5:
						//printf("Lyric:\"");
						break;
					case 6:
						//printf("Marker:\"");
						break;
					case 7:
						//printf("Cue point:\"");
						break;
					case 8:
						//printf("Patch name:\"");
						break;
					case 9:
						//printf("Port name:\"");
						break;
					}
					while (len)
					{
						/*int c = */fgetc(f);
						//printf("%c", c);
						len--;
					}
					//printf("\"\n");
					break;
				case 0x2f:
				{
					trackend = 1;
					//printf("Track end\n");
				}
				break;
				case 0x58: // time signature
				{
					/*int nn = */fgetc(f);
					/*int dd = */fgetc(f);
					/*int cc = */fgetc(f);
					/*int bb = */fgetc(f);
					//printf("Time sig: %d:%d, metronome:%d, quarter:%d\n", nn, dd, cc, bb);
				}
				break;
				case 0x59: // key signature
				{
					/*int sf = */fgetc(f);
					/*int mi = */fgetc(f);
					//printf("Key sig: %d %s, %s\n", abs(sf), sf == 0 ? "c" : (sf < 0 ? "flat" : "sharp"), mi ? "minor" : "major");
				}
				break;
				case 0x51: // tempo
				{
					int t = 0;
					t = fgetc(f) << 16;
					t |= fgetc(f) << 8;
					t |= fgetc(f);
					//printf("Tempo: quarter is %dus (%3.3fs) long - BPM = %3.3f\n", t, t / 1000000.0f, 60000000.0f / t);
					uspertick = t / ppqn;
				}
				break;
				case 0x21: // obsolete: midi port
				{
					/*int pp = */fgetc(f);
					//printf("[obsolete] midi port: %d\n", pp);
				}
				break;
				case 0x20: // obsolete: midi channel
				{
					/*int cc = */fgetc(f);
					//printf("[obsolete] midi channel: %d\n", cc);
				}
				break;
				case 0x54: // SMPTE offset
				{
					/*int hr = */fgetc(f);
					/*int mn = */fgetc(f);
					/*int se = */fgetc(f);
					/*int fr = */fgetc(f);
					/*int ff = */fgetc(f);
					//printf("SMPTE Offset: %dh %dm %ds %dfr %dff\n", hr, mn, se, fr, ff);
				}
				break;
				case 0x7f: // Proprietary event
				{
					//printf("Proprietary event ");
					while (len)
					{
						/*int d = */fgetc(f);
						//printf("%02X ", d);
						len--;
					}
					printf("\n");
				}
				break;
				default:
					//printf("meta command %02x %d\n", data1, len);
					while (len)
					{
						fgetc(f);
						len--;
					}
				}
			}
			else
			{
				if (data1 & 0x80) // new command?
				{
					command = data1;
					data1 = fgetc(f);
				}
				int data2 = 0;

				switch (command & 0xf0)
				{
				case 0x80: // note off
				{
					data2 = fgetc(f);
					//printf("Note off: channel %d, Oct %d Note %s Velocity %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0x90: // note on
				{
					data2 = fgetc(f);
					//printf("Note on: channel %d, Oct %d Note %s Velocity %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0xa0: // Note aftertouch
				{
					data2 = fgetc(f);
					//printf("Aftertouch: channel %d, Oct %d, Note %s Aftertouch %d\n", command & 0xf, (data1 / 12) - 1, note[data1 % 12], data2);
				}
				break;
				case 0xb0: // Controller
				{
					data2 = fgetc(f);
					//printf("Controller: channel %d, Controller %s Value %d\n", command & 0xf, controller[data1], data2);
				}
				break;
				case 0xc0: // program change
				{
					//printf("Program change: channel %d, program %d\n", command & 0xf, data1);
				}
				break;
				case 0xd0: // Channel aftertouch
				{
					//printf("Channel aftertouch: channel %d, Aftertouch %d\n", command & 0xf, data1);
				}
				break;
				case 0xe0: // Pitch bend
				{
					data2 = fgetc(f);
					//printf("Pitchbend: channel %d, Pitch %d\n", command & 0xf, data1 + (data2 << 7));
				}
				break;
				case 0xf0: // general / immediate
				{
					switch (command)
					{
					case 0xf0: // SysEx
					{
						//printf("SysEx ");
						while (data1 != 0xf7)
						{
							//printf("%02X ", data1);
							data1 = fgetc(f);
						}
						//printf("\n");
						// universal sysexes of note:
						// f0 (05) 7e 7f 09 01 f7 = "general midi enable"
						// f0 (05) 7e 7f 09 00 f7 = "general midi disable"
						// f0 (07) 7f 7f 04 01 ll mm f7 = "master volume", ll mm = 14bit value
						// spec doesn't say that the length byte should be there,
						// but it appears to be (the ones in brackets)
					}
					break;
					case 0xf1: // MTC quater frame
					{
						/*int dd = */fgetc(f);
						//printf("MTC quater frame %d\n", dd);
					}
					break;
					case 0xf2: // Song position pointer
					{
						/*int data1 = */fgetc(f);
						/*int data2 = */fgetc(f);
						//printf("Song position pointer %d\n", data1 + (data2 << 7));
					}
					break;
					case 0xf3: // Song select
					{
						/*int song = */fgetc(f);
						//printf("Song select %d\n", song);
					}
					break;
					case 0xf6: // Tuning request
						//printf("Tuning request\n");
						break;
					case 0xf8: // MIDI clock
						//printf("MIDI clock\n");
						break;
					case 0xf9: // MIDI Tick
						//printf("MIDI Tick\n");
						break;
					case 0xfa: // MIDI start
						//printf("MIDI start\n");
						break;
					case 0xfc:
						//printf("MIDI stop\n");
						break;
					case 0xfb:
						//printf("MIDI continue\n");
						break;
					case 0xfe:
						//printf("Active sense\n");
						break;
					case 0xff:
						//printf("Reset\n");
						break;

					default:
					{
						printf("Midi loader: Unknown: command 0x%02x, data 0x%02x\n", command, data1);
						return -1;
					}
					break;
					}
				}
				break;
				default:
				{
					printf("Midi loader: Unknown: command 0x%02x, data 0x%02x\n", command, data1);
					return -1;
				}
				break;
				}
				if ((command & 0xf0) != 0xf0)
				{
					MidiEvent e;
					e.time = ((double)time * (double)uspertick) / 1000000.0;
					e.d0 = (unsigned char)command;
					e.d1 = (unsigned char)data1;
					e.d2 = (unsigned char)data2;
					gMidiData.push_back(e);
				}
			}
		}

		tracks--;
	}
	fclose(f);
	sort(gMidiData.begin(), gMidiData.end(), [](const MidiEvent& l, const MidiEvent& r) { return l.time < r.time; });
	printf("MIDI loader: %s loaded successfully, %d events found.\n", filename, (int)gMidiData.size());

	return 0;
}


static void loadmidi(const char* fn)
{
	gMidiData.clear();
	if (parsemidi(fn))
		gMidiData.clear();
}

int gMidiplayer_mode = 0;
double gMidiplayer_time = 0;
int gMidiplayer_event = 0;

void midiplayer_tick()
{
	if (gMidiplayer_mode == 0 || gMidiData.size() == 0)
		return;
	gMidiplayer_time += dt();

	while (gMidiData[gMidiplayer_event].time < gMidiplayer_time)
	{
		std::vector<unsigned char> msg;
		msg.push_back(gMidiData[gMidiplayer_event].d0);
		msg.push_back(gMidiData[gMidiplayer_event].d1);
		msg.push_back(gMidiData[gMidiplayer_event].d2);
		midicallback(gMidiplayer_time, &msg, 0);
		gMidiplayer_event++;
		if (gMidiplayer_event >= gMidiData.size())
		{
			gMidiplayer_time = 0;
			gMidiplayer_event = 0;
		}
	}
}

void do_show_midiplayer_window()
{
	ImGui::Begin("Midi Player", &gShowMidiPlayerWindow, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

	if (ImGui::Button("Load"))
	{
		const char* filterpatterns[2] = { "*.mid", "*.smf" };
		const char* fn = tinyfd_openFileDialog("Open midi file", "", 2, filterpatterns, "Standard midi file", 0);

		if (fn)
		{
			SDL_LockMutex(gAudioMutex);
			loadmidi(fn);
			gMidiplayer_time = 0;
			gMidiplayer_mode = 0;
			gMidiplayer_event = 0;
			SDL_UnlockMutex(gAudioMutex);
			warm_reset();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("<<"))
	{
		SDL_LockMutex(gAudioMutex);
		gMidiplayer_time = 0;
		gMidiplayer_mode = 0;
		gMidiplayer_event = 0;
		SDL_UnlockMutex(gAudioMutex);
	}
	ImGui::SameLine();
	if (ImGui::Button("="))
	{
		SDL_LockMutex(gAudioMutex);
		gMidiplayer_mode = 0;
		SDL_UnlockMutex(gAudioMutex);
	}
	ImGui::SameLine();
	if (ImGui::Button(">"))
	{
		if (gMidiData.size() != 0)
		{
			SDL_LockMutex(gAudioMutex);
			gMidiplayer_mode = 1;
			SDL_UnlockMutex(gAudioMutex);
		}
	}
	ImGui::SameLine();
	//ImGui::Button(">>");	ImGui::SameLine();
	double maxt = 0;
	if (gMidiData.size())
		maxt = gMidiData[gMidiData.size() - 1].time;
	int maxt_100ths = (int)(maxt * 100);
	int curt_100ths = (int)(gMidiplayer_time * 100);
	ImGui::Text("%02d:%02d.%02d", curt_100ths / (100 * 60), (curt_100ths / 100) % 60, curt_100ths % 100);
	float pos = 0;
	if (maxt_100ths)
		pos = (float)curt_100ths / (float)maxt_100ths;
	if (ImGui::SliderFloat("###Midipos", &pos, 0, 1, ""))
	{
		gMidiplayer_time = pos * maxt;
		gMidiplayer_event = 0;
		while (gMidiData[gMidiplayer_event].time < gMidiplayer_time)
			gMidiplayer_event++;
		warm_reset();
	}
	ImGui::Text("Len: %02d:%02d.%02d, %d events", maxt_100ths / (100 * 60), (maxt_100ths / 100) % 60, maxt_100ths % 100, gMidiData.size());
	ImGui::End();


}
