/* Author: Peter Sovietov */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ayumi.h"
#include "load_text.h"

#define WAVE_FORMAT_IEEE_FLOAT 3

int save_wave_file(const char* name, float* data,
  int sample_rate, int channel_count, int sample_count) {
  FILE* f;
  char header[58];
  int sample_size = sizeof(float);  
  int data_size = sample_size * channel_count * sample_count;
  int pad = data_size & 1;
  memcpy(&header[0], "RIFF", 4);
  *((int*) (header + 4)) = 50 + data_size + pad;
  memcpy(&header[8], "WAVE", 4);
  memcpy(&header[12], "fmt ", 4);
  *((int*) (header + 16)) = 18;
  *((short*) (header + 20)) = WAVE_FORMAT_IEEE_FLOAT;
  *((short*) (header + 22)) = (short) channel_count;
  *((int*) (header + 24)) = sample_rate;
  *((int*) (header + 28)) = sample_rate * sample_size * channel_count;
  *((short*) (header + 32)) = (short) (sample_size * channel_count);
  *((short*) (header + 34)) = (short) (8 * sample_size);
  *((short*) (header + 36)) = 0;
  memcpy(&header[38], "fact", 4);
  *((int*) (header + 42)) = 4;
  *((int*) (header + 46)) = channel_count * sample_count;
  memcpy(&header[50], "data", 4);
  *((int*) (header + 54)) = data_size;
  f = fopen(name, "wb");
  if (f == NULL) {
    return 0;
  }
  fwrite(header, 1, sizeof(header), f);
  fwrite(data, 1, data_size, f);
  if (pad) {
    fwrite(&pad, 1, 1, f);
  }
  fclose(f);
  return 1;
}

void update_ayumi_state(struct ayumi* ay, int* r) {
  ayumi_set_tone(ay, 0, (r[1] << 8) | r[0]);
  ayumi_set_tone(ay, 1, (r[3] << 8) | r[2]);
  ayumi_set_tone(ay, 2, (r[5] << 8) | r[4]);
  ayumi_set_noise(ay, r[6]);
  ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, r[8] >> 4);
  ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, r[9] >> 4);
  ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, r[10] >> 4);
  ayumi_set_volume(ay, 0, r[8] & 0xf);
  ayumi_set_volume(ay, 1, r[9] & 0xf);
  ayumi_set_volume(ay, 2, r[10] & 0xf);
  ayumi_set_envelope(ay, (r[12] << 8) | r[11]);
  if (r[13] != 255) {
    ayumi_set_envelope_shape(ay, r[13]);
  }
}

void ayumi_render(struct ayumi* ay, struct text_data* t, float* sample_data) {
  int frame = 0;
  double isr_step = t->frame_rate / t->sample_rate;
  double isr_counter = 1;
  float* out = sample_data;
  while (frame < t->frame_count) {
    isr_counter += isr_step;
    if (isr_counter >= 1) {
      isr_counter -= 1;
      update_ayumi_state(ay, &t->frame_data[frame * 16]);
      frame += 1;
    }
    ayumi_process(ay);
    if (t->dc_filter_on) {
      ayumi_remove_dc(ay);
    }
    out[0] = (float) (ay->left * t->volume);
    out[1] = (float) (ay->right * t->volume);
    out += 2;
  }
}

void set_default_text_data(struct text_data* t) {
  memset(t, 0, sizeof(struct text_data));
  t->sample_rate = 44100;
  t->eqp_stereo_on = 0;
  t->dc_filter_on = 1;
  t->is_ym = 1;
  t->clock_rate = 2000000;
  t->frame_rate = 50;
}

int main(int argc, char** argv) {
  int sample_count;
  float* sample_data;
  struct text_data t;
  struct ayumi ay;
  if (argc != 3) {
    printf("ayumi_render input.txt output.wav\n");
    return 1;
  }
  set_default_text_data(&t);
  if(!load_text_file(argv[1], &t)) {
    printf("Load error\n");
    return 1;
  }
  sample_count = (int) ((t.sample_rate / t.frame_rate) * t.frame_count);
  if (sample_count == 0) {
    printf("No frames\n");
    return 1;
  }
  sample_data = (float*) malloc(sample_count * sizeof(float) * 2);
  if (sample_data == NULL) {
    printf("Memory allocation error\n");
    return 1;
  }
  if (!ayumi_configure(&ay, t.is_ym, t.clock_rate, t.sample_rate)) {
    printf("ayumi_configure error (wrong sample rate?)\n");
    return 1;
  }
  if (t.pan[0] >= 0) {
    ayumi_set_pan(&ay, 0, t.pan[0], t.eqp_stereo_on);
  }
  if (t.pan[1] >= 0) {
    ayumi_set_pan(&ay, 1, t.pan[1], t.eqp_stereo_on);
  }
  if (t.pan[2] >= 0) {
    ayumi_set_pan(&ay, 2, t.pan[2], t.eqp_stereo_on);
  }
  ayumi_render(&ay, &t, sample_data);
  if (!save_wave_file(argv[2], sample_data, t.sample_rate, 2, sample_count)) {
    printf("Save error\n");
    return 1;
  }
  return 0;
}
