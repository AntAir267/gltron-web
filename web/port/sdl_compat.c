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
