/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* The slice of the SDL 1.2 API GLtron uses, as the native build provides it
   on top of SDL3 (sdl1.c). The names are prefixed so sdl1.c can include this
   next to SDL3's headers; SDL.h maps them back to the names GLtron calls. */
#ifndef NATIVE_SDL1_H
#define NATIVE_SDL1_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t sdl1_Uint8;
typedef int16_t sdl1_Sint16;
typedef uint16_t sdl1_Uint16;
typedef uint32_t sdl1_Uint32;

/* ---- events ---- */
enum { SDL1_NOEVENT, SDL1_KEYDOWN, SDL1_KEYUP, SDL1_JOYAXISMOTION, SDL1_JOYBUTTONDOWN,
       SDL1_JOYBUTTONUP, SDL1_MOUSEBUTTONDOWN, SDL1_MOUSEBUTTONUP, SDL1_MOUSEMOTION,
       SDL1_QUIT, SDL1_VIDEORESIZE };

typedef struct { int sym; } sdl1_keysym;
typedef union {
  int type;
  struct { int type; sdl1_keysym keysym; } key;
  struct { int type; int which, axis, value; } jaxis;
  struct { int type; int which, button; } jbutton;
  struct { int type; int button, state, x, y; } button;
  struct { int type; int x, y; } motion;
  struct { int type; int w, h; } resize;
} sdl1_Event;

int sdl1_PollEvent(sdl1_Event *event);
const char *sdl1_GetKeyName(int key);
void sdl1_EnableKeyRepeat(int delay, int interval);

/* ---- start-up ---- */
#define SDL1_INIT_AUDIO 1
#define SDL1_INIT_VIDEO 2
#define SDL1_INIT_JOYSTICK 4
int sdl1_Init(int flags);
void sdl1_QuitSubSystem(int flags);
void sdl1_Quit(void);
const char *sdl1_GetError(void);
sdl1_Uint32 sdl1_GetTicks(void);
void sdl1_Delay(sdl1_Uint32 ms);

/* ---- joysticks: controllers go through pad.c instead, so GLtron sees none ---- */
int sdl1_NumJoysticks(void);

/* ---- video ---- */
#define SDL1_OPENGL 1
#define SDL1_FULLSCREEN 2
enum { SDL1_GL_DOUBLEBUFFER, SDL1_GL_DEPTH_SIZE, SDL1_GL_STENCIL_SIZE };
typedef struct { int w, h, pitch; void *pixels; } sdl1_Surface;
sdl1_Surface *sdl1_SetVideoMode(int w, int h, int bpp, int flags);
int sdl1_GL_SetAttribute(int attr, int value);
void sdl1_GL_SwapBuffers(void);
void sdl1_ShowCursor(int show);
sdl1_Surface *sdl1_CreateRGB24Surface(int w, int h);
int sdl1_SaveBMP(sdl1_Surface *surface, const char *filename);
void sdl1_FreeSurface(sdl1_Surface *surface);

/* ---- audio ---- */
#define SDL1_AUDIO_S16 0x8010
#define SDL1_MIX_MAXVOLUME 128
typedef struct {
  int freq;
  sdl1_Uint16 format;
  sdl1_Uint8 channels, silence;
  sdl1_Uint16 samples;
  sdl1_Uint32 size;
  void (*callback)(void *userdata, sdl1_Uint8 *stream, int len);
  void *userdata;
} sdl1_AudioSpec;
int sdl1_OpenAudio(sdl1_AudioSpec *desired, sdl1_AudioSpec *obtained);
void sdl1_PauseAudio(int pause);
void sdl1_CloseAudio(void);
void sdl1_LockAudio(void);
void sdl1_UnlockAudio(void);
void sdl1_MixAudio(sdl1_Uint8 *dst, const sdl1_Uint8 *src, sdl1_Uint32 len, int volume);

typedef struct sdl1_sem sdl1_sem;
sdl1_sem *sdl1_CreateSemaphore(sdl1_Uint32 initial);
void sdl1_DestroySemaphore(sdl1_sem *sem);
int sdl1_SemWait(sdl1_sem *sem);
int sdl1_SemTryWait(sdl1_sem *sem);
int sdl1_SemPost(sdl1_sem *sem);

/* just a file name: SDL_sound (web/port/sdl_sound.c) opens it itself */
typedef struct { char *path; } sdl1_RWops;
sdl1_RWops *sdl1_RWFromFile(const char *file, const char *mode);

#ifdef __cplusplus
}
#endif
#endif
