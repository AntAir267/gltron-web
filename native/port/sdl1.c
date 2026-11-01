/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* SDL 1.2 as GLtron uses it, on top of SDL3: a window with an OpenGL context,
   key and mouse events, a callback audio device. See include/sdl1.h. */
#include <SDL3/SDL.h>
#include "sdl1.h"
#include "native.h"

#include <stdlib.h>
#include <string.h>

/* ---- start-up ---- */
static SDL_InitFlags to_sdl3(int flags) {
  SDL_InitFlags f = 0;
  if(flags & SDL1_INIT_AUDIO) f |= SDL_INIT_AUDIO;
  if(flags & SDL1_INIT_VIDEO) f |= SDL_INIT_VIDEO;
  if(flags & SDL1_INIT_JOYSTICK) f |= SDL_INIT_GAMEPAD;
  return f;
}

int sdl1_Init(int flags) {
  SDL_SetHint(SDL_HINT_APP_ID, "gltron");
  SDL_SetAppMetadata("GLtron", "0.70", "gltron");
  if(!SDL_InitSubSystem(to_sdl3(flags)))
    return -1;
  if(flags & SDL1_INIT_JOYSTICK) {
    native_pad_init();
    native_test_init();
  }
  return 0;
}
void sdl1_QuitSubSystem(int flags) { SDL_QuitSubSystem(to_sdl3(flags)); }
void sdl1_Quit(void) { SDL_Quit(); }
const char *sdl1_GetError(void) { return SDL_GetError(); }
sdl1_Uint32 sdl1_GetTicks(void) { return (sdl1_Uint32) SDL_GetTicks(); }
void sdl1_Delay(sdl1_Uint32 ms) { SDL_Delay(ms); }
int sdl1_NumJoysticks(void) { return 0; }
void sdl1_EnableKeyRepeat(int delay, int interval) { (void) delay; (void) interval; }

/* ---- video ---- */
static SDL_Window *window;
static SDL_GLContext context;
static int want_double = 1, want_depth = 24, want_stencil = 8;
static int pixel_w, pixel_h;

int sdl1_GL_SetAttribute(int attr, int value) {
  switch(attr) {
  case SDL1_GL_DOUBLEBUFFER: want_double = value; break;
  case SDL1_GL_DEPTH_SIZE: want_depth = value; break;
  case SDL1_GL_STENCIL_SIZE: want_stencil = value; break;
  }
  return 0;
}

static int resize_pending;

sdl1_Surface *sdl1_SetVideoMode(int w, int h, int bpp, int flags) {
  static sdl1_Surface surface;
  SDL_DisplayID display;
  const SDL_DisplayMode *mode;
  SDL_WindowFlags wf = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  (void) bpp;

  if(context) SDL_GL_DestroyContext(context);
  if(window) SDL_DestroyWindow(window);
  context = NULL;
  window = NULL;

  /* a window can't be bigger than the screen: the saved size may be left over
     from fullscreen */
  display = SDL_GetPrimaryDisplay();
  mode = display ? SDL_GetDesktopDisplayMode(display) : NULL;
  if(!(flags & SDL1_FULLSCREEN) && mode && (w >= mode->w || h >= mode->h)) {
    w = mode->w * 3 / 4;
    h = mode->h * 3 / 4;
  }
  if(flags & SDL1_FULLSCREEN)
    wf |= SDL_WINDOW_FULLSCREEN;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, want_double);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, want_depth);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, want_stencil);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

  window = SDL_CreateWindow("GLtron", w, h, wf);
  if(!window)
    return NULL;
  context = SDL_GL_CreateContext(window);
  if(!context) {
    SDL_DestroyWindow(window);
    window = NULL;
    return NULL;
  }
  SDL_GL_SetSwapInterval(1);
  SDL_SyncWindow(window);
  /* GLtron learns the real size (fullscreen, hi-DPI) from the next poll */
  SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);
  resize_pending = 1;
  surface.w = pixel_w;
  surface.h = pixel_h;
  return &surface;
}

void sdl1_GL_SwapBuffers(void) {
  native_before_swap();
  if(window) SDL_GL_SwapWindow(window);
}

void sdl1_ShowCursor(int show) {
  if(show) SDL_ShowCursor(); else SDL_HideCursor();
}

sdl1_Surface *sdl1_CreateRGB24Surface(int w, int h) {
  sdl1_Surface *s = calloc(1, sizeof(sdl1_Surface));
  s->w = w;
  s->h = h;
  s->pitch = w * 3;
  s->pixels = malloc(s->pitch * h);
  return s;
}

int sdl1_SaveBMP(sdl1_Surface *s, const char *filename) {
  SDL_Surface *t = SDL_CreateSurfaceFrom(s->w, s->h, SDL_PIXELFORMAT_RGB24, s->pixels, s->pitch);
  int ok = t && SDL_SaveBMP(t, filename);
  SDL_DestroySurface(t);
  return ok ? 0 : -1;
}

void sdl1_FreeSurface(sdl1_Surface *s) {
  free(s->pixels);
  free(s);
}

/* ---- events ---- */
static int key_to_sdl1(SDL_Keycode k) {
  /* printable keys have the same numbers in both */
  if(k < 128) return k;
  switch(k) {
  case SDLK_UP: return 273;
  case SDLK_DOWN: return 274;
  case SDLK_RIGHT: return 275;
  case SDLK_LEFT: return 276;
  case SDLK_INSERT: return 277;
  case SDLK_HOME: return 278;
  case SDLK_END: return 279;
  case SDLK_PAGEUP: return 280;
  case SDLK_PAGEDOWN: return 281;
  case SDLK_F1: return 282; case SDLK_F2: return 283; case SDLK_F3: return 284;
  case SDLK_F4: return 285; case SDLK_F5: return 286; case SDLK_F6: return 287;
  case SDLK_F7: return 288; case SDLK_F8: return 289; case SDLK_F9: return 290;
  case SDLK_F10: return 291; case SDLK_F11: return 292; case SDLK_F12: return 293;
  case SDLK_NUMLOCKCLEAR: return 300;
  case SDLK_CAPSLOCK: return 301;
  case SDLK_SCROLLLOCK: return 302;
  case SDLK_RSHIFT: return 303; case SDLK_LSHIFT: return 304;
  case SDLK_RCTRL: return 305; case SDLK_LCTRL: return 306;
  case SDLK_RALT: return 307; case SDLK_LALT: return 308;
  case SDLK_RGUI: return 311; case SDLK_LGUI: return 312;
  case SDLK_PRINTSCREEN: return 316;
  case SDLK_MENU: return 319;
  case SDLK_KP_0: return 256; case SDLK_KP_1: return 257; case SDLK_KP_2: return 258;
  case SDLK_KP_3: return 259; case SDLK_KP_4: return 260; case SDLK_KP_5: return 261;
  case SDLK_KP_6: return 262; case SDLK_KP_7: return 263; case SDLK_KP_8: return 264;
  case SDLK_KP_9: return 265;
  case SDLK_KP_PERIOD: return 266; case SDLK_KP_DIVIDE: return 267;
  case SDLK_KP_MULTIPLY: return 268; case SDLK_KP_MINUS: return 269;
  case SDLK_KP_PLUS: return 270; case SDLK_KP_ENTER: return 271;
  case SDLK_KP_EQUALS: return 272;
  }
  return 0;
}

const char *sdl1_GetKeyName(int key) {
  static const struct { int key; const char *name; } names[] = {
    { 8, "backspace" }, { 9, "tab" }, { 13, "return" }, { 27, "escape" }, { 32, "space" },
    { 127, "delete" }, { 273, "up" }, { 274, "down" }, { 275, "right" }, { 276, "left" },
    { 277, "insert" }, { 278, "home" }, { 279, "end" }, { 280, "page up" },
    { 281, "page down" }, { 282, "f1" }, { 283, "f2" }, { 284, "f3" }, { 285, "f4" },
    { 286, "f5" }, { 287, "f6" }, { 288, "f7" }, { 289, "f8" }, { 290, "f9" }, { 291, "f10" },
    { 292, "f11" }, { 293, "f12" }, { 300, "num lock" }, { 301, "caps lock" },
    { 302, "scroll lock" }, { 303, "right shift" }, { 304, "left shift" },
    { 305, "right ctrl" }, { 306, "left ctrl" }, { 307, "right alt" }, { 308, "left alt" },
    { 311, "right super" }, { 312, "left super" }, { 316, "print screen" }, { 319, "menu" },
    { 256, "keypad 0" }, { 257, "keypad 1" }, { 258, "keypad 2" }, { 259, "keypad 3" },
    { 260, "keypad 4" }, { 261, "keypad 5" }, { 262, "keypad 6" }, { 263, "keypad 7" },
    { 264, "keypad 8" }, { 265, "keypad 9" }, { 266, "keypad ." }, { 267, "keypad /" },
    { 268, "keypad *" }, { 269, "keypad -" }, { 270, "keypad +" }, { 271, "keypad enter" },
    { 272, "keypad =" },
  };
  static char printable[128][2];
  size_t i;

  for(i = 0; i < sizeof(names) / sizeof(names[0]); i++)
    if(names[i].key == key)
      return names[i].name;
  if(key > 32 && key < 127) {
    printable[key][0] = (char) key;
    printable[key][1] = 0;
    return printable[key];
  }
  return "unknown key";
}

int sdl1_PollEvent(sdl1_Event *out) {
  SDL_Event e;
  static int ticked;

  /* once per poll round: scripted tests and held menu directions */
  if(!ticked) {
    ticked = 1;
    native_test_tick();
    native_pad_tick();
  }
  if(resize_pending) {
    resize_pending = 0;
    out->type = SDL1_VIDEORESIZE;
    out->resize.w = pixel_w;
    out->resize.h = pixel_h;
    return 1;
  }
  while(SDL_PollEvent(&e)) {
    switch(e.type) {
    case SDL_EVENT_QUIT:
      out->type = SDL1_QUIT;
      return 1;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      int sym = key_to_sdl1(e.key.key);
      if(e.key.repeat || !sym)
        break;
      out->type = e.type == SDL_EVENT_KEY_DOWN ? SDL1_KEYDOWN : SDL1_KEYUP;
      out->key.keysym.sym = sym;
      return 1;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      out->type = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? SDL1_MOUSEBUTTONDOWN : SDL1_MOUSEBUTTONUP;
      out->button.button = e.button.button;
      out->button.state = e.button.down;
      out->button.x = (int) e.button.x;
      out->button.y = (int) e.button.y;
      return 1;
    case SDL_EVENT_MOUSE_MOTION:
      out->type = SDL1_MOUSEMOTION;
      out->motion.x = (int) e.motion.x;
      out->motion.y = (int) e.motion.y;
      return 1;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      pixel_w = e.window.data1;
      pixel_h = e.window.data2;
      out->type = SDL1_VIDEORESIZE;
      out->resize.w = pixel_w;
      out->resize.h = pixel_h;
      return 1;
    default:
      native_pad_event(&e);
      break;
    }
  }
  ticked = 0;
  return 0;
}

/* ---- audio ---- */
static SDL_AudioStream *stream;
static sdl1_AudioSpec spec;

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *s, int additional, int total) {
  /* GLtron's mixer fills whole buffers */
  static sdl1_Uint8 buf[8192];
  int chunk = spec.samples * spec.channels * 2;
  (void) userdata; (void) total;
  if(chunk > (int) sizeof(buf)) chunk = sizeof(buf);
  while(additional > 0) {
    spec.callback(spec.userdata, buf, chunk);
    SDL_PutAudioStreamData(s, buf, chunk);
    additional -= chunk;
  }
}

int sdl1_OpenAudio(sdl1_AudioSpec *desired, sdl1_AudioSpec *obtained) {
  SDL_AudioSpec want;
  spec = *desired;
  want.format = SDL_AUDIO_S16;
  want.channels = desired->channels;
  want.freq = desired->freq;
  stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, audio_callback, NULL);
  if(!stream)
    return -1;
  if(obtained) *obtained = spec;
  return 0;
}

void sdl1_PauseAudio(int pause) {
  if(!stream) return;
  if(pause) SDL_PauseAudioStreamDevice(stream); else SDL_ResumeAudioStreamDevice(stream);
}
void sdl1_CloseAudio(void) {
  if(stream) SDL_DestroyAudioStream(stream);
  stream = NULL;
}
void sdl1_LockAudio(void) { if(stream) SDL_LockAudioStream(stream); }
void sdl1_UnlockAudio(void) { if(stream) SDL_UnlockAudioStream(stream); }

/* SDL 1.2's SDL_MixAudio for the signed 16-bit device GLtron opens */
void sdl1_MixAudio(sdl1_Uint8 *dst, const sdl1_Uint8 *src, sdl1_Uint32 len, int volume) {
  sdl1_Sint16 *d = (sdl1_Sint16 *) dst;
  const sdl1_Sint16 *s = (const sdl1_Sint16 *) src;
  sdl1_Uint32 n = len / 2;

  if(volume == 0)
    return;
  while(n--) {
    int v = *d + (*s++ * volume) / SDL1_MIX_MAXVOLUME;
    if(v > 32767) v = 32767;
    else if(v < -32768) v = -32768;
    *d++ = (sdl1_Sint16) v;
  }
}

struct sdl1_sem { SDL_Semaphore *sem; };
sdl1_sem *sdl1_CreateSemaphore(sdl1_Uint32 initial) {
  sdl1_sem *s = malloc(sizeof(sdl1_sem));
  s->sem = SDL_CreateSemaphore(initial);
  return s;
}
void sdl1_DestroySemaphore(sdl1_sem *s) {
  SDL_DestroySemaphore(s->sem);
  free(s);
}
int sdl1_SemWait(sdl1_sem *s) { SDL_WaitSemaphore(s->sem); return 0; }
int sdl1_SemTryWait(sdl1_sem *s) { return SDL_TryWaitSemaphore(s->sem) ? 0 : 1; }
int sdl1_SemPost(sdl1_sem *s) { SDL_SignalSemaphore(s->sem); return 0; }

sdl1_RWops *sdl1_RWFromFile(const char *file, const char *mode) {
  sdl1_RWops *rw = malloc(sizeof(sdl1_RWops));
  (void) mode;
  rw->path = strdup(file);
  return rw;
}
