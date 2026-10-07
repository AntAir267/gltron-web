/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* Minimal SDL_sound for the web build: loads a whole PCM WAV file that is
   already in the mixer's format. build.sh converts the game's WAVs to
   22050 Hz signed 16-bit stereo, which is what GLtron opens the device with. */
#include "SDL_sound.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *error = NULL;

static const char *wav_extensions[] = { "WAV", NULL };
static const Sound_DecoderInfo wav_decoder = {
  wav_extensions, "PCM WAV (web build)", "gltron-web", ""
};
static const Sound_DecoderInfo *decoders[] = { &wav_decoder, NULL };

int Sound_Init(void) { return 1; }
int Sound_Quit(void) { return 1; }
const char *Sound_GetError(void) { return error ? error : ""; }
const Sound_DecoderInfo **Sound_AvailableDecoders(void) { return decoders; }

static Uint32 le32(const Uint8 *p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | ((Uint32)p[3] << 24);
}
static Uint16 le16(const Uint8 *p) { return p[0] | (p[1] << 8); }

Sound_Sample *Sound_NewSample(SDL_RWops *rw, const char *ext,
                              Sound_AudioInfo *desired, Uint32 bufferSize) {
  /* Emscripten's SDL_RWops can't be read from C */
  error = "Sound_NewSample: use Sound_NewSampleFromFile";
  return NULL;
}

Sound_Sample *Sound_NewSampleFromFile(const char *filename,
                                      Sound_AudioInfo *desired,
                                      Uint32 bufferSize) {
  FILE *f;
  long size;
  Uint8 *file = NULL, *p, *end, *data = NULL;
  Uint32 data_size = 0;
  Sound_AudioInfo actual = { 0, 0, 0 };
  Sound_Sample *sample;

  f = fopen(filename, "rb");
  if(f == NULL) {
    error = "can't open file";
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  file = malloc(size);
  if(fread(file, 1, size, f) != (size_t)size) {
    fclose(f);
    free(file);
    error = "can't read file";
    return NULL;
  }
  fclose(f);

  if(size < 12 || memcmp(file, "RIFF", 4) || memcmp(file + 8, "WAVE", 4)) {
    free(file);
    error = "not a WAV file";
    return NULL;
  }
  end = file + size;
  for(p = file + 12; p + 8 <= end; p += 8 + ((le32(p + 4) + 1) & ~1u)) {
    Uint32 len = le32(p + 4);
    if(p + 8 + len > end)
      len = end - (p + 8);
    if(!memcmp(p, "fmt ", 4) && len >= 16) {
      if(le16(p + 8) != 1 || le16(p + 22) != 16) {
        free(file);
        error = "WAV is not 16-bit PCM";
        return NULL;
      }
      actual.format = AUDIO_S16LSB;
      actual.channels = le16(p + 10);
      actual.rate = le32(p + 12);
    } else if(!memcmp(p, "data", 4)) {
      data = p + 8;
      data_size = len;
    }
  }
  if(data == NULL || actual.format == 0) {
    free(file);
    error = "WAV has no fmt or data chunk";
    return NULL;
  }
  if(desired && (actual.channels != desired->channels ||
                 actual.rate != desired->rate)) {
    fprintf(stderr, "[sound] %s is %d ch / %d Hz, the mixer wants %d / %d\n",
            filename, actual.channels, actual.rate,
            desired->channels, desired->rate);
  }

  sample = calloc(1, sizeof(Sound_Sample));
  sample->decoder = &wav_decoder;
  sample->actual = actual;
  sample->desired = desired ? *desired : actual;
  sample->buffer = malloc(data_size);
  memcpy(sample->buffer, data, data_size);
  sample->buffer_size = data_size;
  sample->flags = SOUND_SAMPLEFLAG_EOF;
  free(file);
  return sample;
}

void Sound_FreeSample(Sound_Sample *sample) {
  if(sample == NULL)
    return;
  free(sample->buffer);
  free(sample);
}

/* the whole file is decoded on load */
Uint32 Sound_Decode(Sound_Sample *sample) { return 0; }
Uint32 Sound_DecodeAll(Sound_Sample *sample) { return sample->buffer_size; }
