/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* What GLtron's #include "SDL.h" gets in the native build: see sdl1.h. */
#ifndef NATIVE_SDL_H
#define NATIVE_SDL_H

#include "sdl1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef sdl1_Uint8 Uint8;
typedef sdl1_Sint16 Sint16;
typedef sdl1_Uint16 Uint16;
typedef sdl1_Uint32 Uint32;
typedef int32_t Sint32;

/* GLtron creates a mutex for each sound source and never uses it */
typedef int SDL_mutex;
#define SDL_CreateMutex() ((SDL_mutex *) 0)
#define SDL_DestroyMutex(m) ((void) (m))

#define SDL_Event sdl1_Event
#define SDL_KEYDOWN SDL1_KEYDOWN
#define SDL_KEYUP SDL1_KEYUP
#define SDL_JOYAXISMOTION SDL1_JOYAXISMOTION
#define SDL_JOYBUTTONDOWN SDL1_JOYBUTTONDOWN
#define SDL_JOYBUTTONUP SDL1_JOYBUTTONUP
#define SDL_MOUSEBUTTONDOWN SDL1_MOUSEBUTTONDOWN
#define SDL_MOUSEBUTTONUP SDL1_MOUSEBUTTONUP
#define SDL_MOUSEMOTION SDL1_MOUSEMOTION
#define SDL_QUIT SDL1_QUIT
#define SDL_VIDEORESIZE SDL1_VIDEORESIZE
#define SDL_PRESSED 1
#define SDL_RELEASED 0
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
#define SDL_ENABLE 1
#define SDL_DISABLE 0

#define SDL_PollEvent sdl1_PollEvent
#define SDL_GetKeyName(k) ((char *) sdl1_GetKeyName(k))
#define SDL_EnableKeyRepeat sdl1_EnableKeyRepeat
#define SDL_INIT_AUDIO SDL1_INIT_AUDIO
#define SDL_INIT_VIDEO SDL1_INIT_VIDEO
#define SDL_INIT_JOYSTICK SDL1_INIT_JOYSTICK
#define SDL_Init sdl1_Init
#define SDL_InitSubSystem sdl1_Init
#define SDL_QuitSubSystem sdl1_QuitSubSystem
#define SDL_Quit sdl1_Quit
#define SDL_GetError sdl1_GetError
#define SDL_GetTicks sdl1_GetTicks
#define SDL_Delay sdl1_Delay
#define SDL_NumJoysticks sdl1_NumJoysticks
#define SDL_JoystickOpen(i) ((void) (i))
#define SDL_JoystickEventState(s) ((void) (s))

#define SDL_Surface sdl1_Surface
#define SDL_OPENGL SDL1_OPENGL
#define SDL_FULLSCREEN SDL1_FULLSCREEN
#define SDL_GL_DOUBLEBUFFER SDL1_GL_DOUBLEBUFFER
#define SDL_GL_DEPTH_SIZE SDL1_GL_DEPTH_SIZE
#define SDL_GL_STENCIL_SIZE SDL1_GL_STENCIL_SIZE
#define SDL_SetVideoMode sdl1_SetVideoMode
#define SDL_GL_SetAttribute sdl1_GL_SetAttribute
#define SDL_GL_SwapBuffers sdl1_GL_SwapBuffers
#define SDL_ShowCursor sdl1_ShowCursor
#define SDL_SetGamma(r, g, b) ((void) 0)
#define SDL_WM_SetCaption(a, b) ((void) 0)
#define SDL_WM_GrabInput(m) ((void) 0)
#define SDL_WarpMouse(x, y) ((void) 0)
#define SDL_SWSURFACE 0
#define SDL_LIL_ENDIAN 1234
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#define SDL_CreateRGBSurface(flags, w, h, depth, r, g, b, a) sdl1_CreateRGB24Surface(w, h)
#define SDL_SaveBMP sdl1_SaveBMP
#define SDL_FreeSurface sdl1_FreeSurface

#define SDL_AudioSpec sdl1_AudioSpec
#define AUDIO_S16SYS SDL1_AUDIO_S16
#define AUDIO_S16LSB SDL1_AUDIO_S16
#define SDL_MIX_MAXVOLUME SDL1_MIX_MAXVOLUME
#define SDL_OpenAudio sdl1_OpenAudio
#define SDL_PauseAudio sdl1_PauseAudio
#define SDL_CloseAudio sdl1_CloseAudio
#define SDL_LockAudio sdl1_LockAudio
#define SDL_UnlockAudio sdl1_UnlockAudio
#define SDL_MixAudio sdl1_MixAudio
#define SDL_sem sdl1_sem
#define SDL_CreateSemaphore sdl1_CreateSemaphore
#define SDL_DestroySemaphore sdl1_DestroySemaphore
#define SDL_SemWait sdl1_SemWait
#define SDL_SemTryWait sdl1_SemTryWait
#define SDL_SemPost sdl1_SemPost
#define SDL_RWops sdl1_RWops
#define SDL_RWFromFile sdl1_RWFromFile

/* SDL 1.2's key numbers: GLtron's saved settings and joystick codes (512 and
   up) are built around them. sdl1.c translates SDL3's keys to these. */
enum {
  SDLK_BACKSPACE = 8, SDLK_TAB = 9, SDLK_RETURN = 13, SDLK_ESCAPE = 27, SDLK_SPACE = 32,
  SDLK_DELETE = 127,
  SDLK_KP0 = 256, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
  SDLK_KP8, SDLK_KP9, SDLK_KP_PERIOD, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS,
  SDLK_KP_PLUS, SDLK_KP_ENTER, SDLK_KP_EQUALS,
  SDLK_UP = 273, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT, SDLK_INSERT, SDLK_HOME, SDLK_END,
  SDLK_PAGEUP, SDLK_PAGEDOWN,
  SDLK_F1 = 282, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9,
  SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14, SDLK_F15,
  SDLK_NUMLOCK = 300, SDLK_CAPSLOCK, SDLK_SCROLLOCK, SDLK_RSHIFT, SDLK_LSHIFT, SDLK_RCTRL,
  SDLK_LCTRL, SDLK_RALT, SDLK_LALT,
  SDLK_RSUPER = 311, SDLK_LSUPER, SDLK_PRINT = 316, SDLK_MENU = 319
};

#endif
