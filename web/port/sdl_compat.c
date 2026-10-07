/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* SDL 1.2 functions GLtron's mixer uses that Emscripten's SDL 1 lacks.
   The page is single-threaded, so the semaphores never block. */
#include "SDL.h"

/* SDL 1.2's SDL_MixAudio for the AUDIO_S16SYS device GLtron opens */
void SDL_MixAudio(Uint8 *dst, const Uint8 *src, Uint32 len, int volume) {
  Sint16 *d = (Sint16 *)dst;
  const Sint16 *s = (const Sint16 *)src;
  Uint32 n = len / 2;

  if(volume == 0)
    return;
  while(n--) {
    int v = *d + (*s++ * volume) / SDL_MIX_MAXVOLUME;
    if(v > 32767)
      v = 32767;
    else if(v < -32768)
      v = -32768;
    *d++ = (Sint16)v;
  }
}

static int dummy_semaphore;

SDL_sem *SDL_CreateSemaphore(Uint32 initial_value) {
  return (SDL_sem *)&dummy_semaphore;
}
void SDL_DestroySemaphore(SDL_sem *sem) {}
int SDL_SemWait(SDL_sem *sem) { return 0; }
int SDL_SemTryWait(SDL_sem *sem) { return 0; }
int SDL_SemPost(SDL_sem *sem) { return 0; }

/* Emscripten's SDL 1 only names a-z and 0-9, and GLtron's Configure Keys
   menu shows key names, so name every key Emscripten reports (see keyCodes
   in its libsdl.js). Special keys are SDL scancodes with bit 10 set. */
#define SCAN(code) ((code) | (1 << 10))

static const struct { int key; const char *name; } key_names[] = {
  { 8, "backspace" }, { 9, "tab" }, { 13, "return" }, { 27, "escape" },
  { 32, "space" }, { 127, "delete" }, { 316, "print screen" },
  { SCAN(82), "up" }, { SCAN(81), "down" }, { SCAN(80), "left" }, { SCAN(79), "right" },
  { SCAN(73), "insert" }, { SCAN(74), "home" }, { SCAN(77), "end" },
  { SCAN(75), "page up" }, { SCAN(78), "page down" },
  { SCAN(58), "f1" }, { SCAN(59), "f2" }, { SCAN(60), "f3" }, { SCAN(61), "f4" },
  { SCAN(62), "f5" }, { SCAN(63), "f6" }, { SCAN(64), "f7" }, { SCAN(65), "f8" },
  { SCAN(66), "f9" }, { SCAN(67), "f10" }, { SCAN(68), "f11" }, { SCAN(69), "f12" },
  { SCAN(104), "f13" }, { SCAN(105), "f14" }, { SCAN(106), "f15" },
  { SCAN(98), "keypad 0" }, { SCAN(89), "keypad 1" }, { SCAN(90), "keypad 2" },
  { SCAN(91), "keypad 3" }, { SCAN(92), "keypad 4" }, { SCAN(93), "keypad 5" },
  { SCAN(94), "keypad 6" }, { SCAN(95), "keypad 7" }, { SCAN(96), "keypad 8" },
  { SCAN(97), "keypad 9" }, { SCAN(99), "keypad ." }, { SCAN(84), "keypad /" },
  { SCAN(85), "keypad *" }, { SCAN(86), "keypad -" }, { SCAN(87), "keypad +" },
  { SCAN(83), "num lock" }, { SCAN(57), "caps lock" },
  { SCAN(225), "left shift" }, { SCAN(229), "right shift" },
  { SCAN(224), "left ctrl" }, { SCAN(228), "right ctrl" },
  { SCAN(226), "left alt" }, { SCAN(230), "right alt" },
  { SCAN(227), "left super" }, { SCAN(231), "right super" }, { SCAN(101), "menu" },
};

const char *SDL_GetKeyName(SDL_Keycode key) {
  static char printable[128][2];
  unsigned i;

  for(i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++)
    if(key_names[i].key == key)
      return key_names[i].name;
  if(key > 32 && key < 127) {     /* letters arrive lowercase */
    printable[key][0] = (char)key;
    printable[key][1] = 0;
    return printable[key];
  }
  return "unknown key";
}
