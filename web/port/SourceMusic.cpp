/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* Web replacement for nebu/audio/SourceMusic.cpp.
   Instead of decoding music into the mixer, hand it to the page
   (Module.music in shell.js), which streams it with an <audio> element.
   Idle() runs every frame and pushes any change in what should be playing. */
#include "audio/nebu_SourceMusic.h"

#include <emscripten.h>
#include <string.h>
#include <stdlib.h>

EM_JS(void, web_music_sync, (const char *name, int playing, float volume, int loop), {
  if (Module.music) Module.music.sync(UTF8ToString(name), !!playing, volume, !!loop);
});

namespace Sound {
  /* only the most recently loaded track talks to the page */
  static SourceMusic *current = NULL;
  static char *sent_name = NULL;
  static int sent_playing = -1;
  static float sent_volume = -1;

  static void sync(const char *name, int playing, float volume, int loop) {
    if(sent_name && !strcmp(sent_name, name) &&
       playing == sent_playing && volume == sent_volume)
      return;
    free(sent_name);
    sent_name = strdup(name);
    sent_playing = playing;
    sent_volume = volume;
    web_music_sync(name, playing, volume, loop);
  }

  SourceMusic::SourceMusic(System *system) {
    _system = system;
    _sample = NULL;
    _buffer = NULL;
    _filename = NULL;
    _rwops = NULL;
  }

  SourceMusic::~SourceMusic() {
    if(current == this) {
      sync("", 0, 0, 0);
      current = NULL;
    }
    free(_filename);
  }

  void SourceMusic::CreateSample(void) { }
  void SourceMusic::CleanUp(void) { }

  void SourceMusic::Load(char *filename) {
    /* the page knows where the music lives; pass just the file name */
    const char *base = strrchr(filename, '/');
    free(_filename);
    _filename = strdup(base ? base + 1 : filename);
    current = this;
    Idle();
  }

  int SourceMusic::Mix(Uint8 *data, int len) { return 0; }

  void SourceMusic::Idle(void) {
    if(current != this || _filename == NULL)
      return;
    sync(_filename, _isPlaying && _system->GetMixMusic(), _volume, _loop);
  }
}
