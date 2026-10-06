/* The slice of the SDL_sound 1.0 API that GLtron uses, for the web build.
   Only whole-file loading of PCM WAV is supported (see sdl_sound.c); music
   is played by the page instead (see SourceMusic.cpp). */
#ifndef WEB_SDL_SOUND_H
#define WEB_SDL_SOUND_H

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SOUND_SAMPLEFLAG_NONE = 0,
  SOUND_SAMPLEFLAG_CANSEEK = 1,
  SOUND_SAMPLEFLAG_EOF = 1 << 29,
  SOUND_SAMPLEFLAG_ERROR = 1 << 30,
  SOUND_SAMPLEFLAG_EAGAIN = 1 << 31
} Sound_SampleFlags;

typedef struct {
  Uint16 format;
  Uint8 channels;
  Uint32 rate;
} Sound_AudioInfo;

typedef struct {
  const char **extensions;
  const char *description;
  const char *author;
  const char *url;
} Sound_DecoderInfo;

typedef struct {
  void *opaque;
  const Sound_DecoderInfo *decoder;
  Sound_AudioInfo desired;
  Sound_AudioInfo actual;
  void *buffer;
  Uint32 buffer_size;
  Sound_SampleFlags flags;
} Sound_Sample;

int Sound_Init(void);
int Sound_Quit(void);
const char *Sound_GetError(void);
const Sound_DecoderInfo **Sound_AvailableDecoders(void);
Sound_Sample *Sound_NewSample(SDL_RWops *rw, const char *ext,
                              Sound_AudioInfo *desired, Uint32 bufferSize);
Sound_Sample *Sound_NewSampleFromFile(const char *filename,
                                      Sound_AudioInfo *desired,
                                      Uint32 bufferSize);
void Sound_FreeSample(Sound_Sample *sample);
Uint32 Sound_Decode(Sound_Sample *sample);
Uint32 Sound_DecodeAll(Sound_Sample *sample);

#ifdef __cplusplus
}
#endif

#endif
