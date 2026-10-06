/* Entry points the host page calls into (see web/shell.js). */
#include "game/gltron.h"

#include <emscripten.h>

/* The page resized the canvas: render at the new size. The WebGL context
   (and every texture in it) survives, so no art is reloaded. */
EMSCRIPTEN_KEEPALIVE void web_resize(int width, int height) {
  if(width < 64 || height < 48)
    return;
  if(width == getSettingi("width") && height == getSettingi("height"))
    return;
  setSettingi("width", width);
  setSettingi("height", height);
  SDL_SetVideoMode(width, height, 0, SDL_OPENGL);
  initGameScreen();
  changeDisplay(-1);
}

/* The page's mute switch, separate from the in-game FX/Music settings so
   it never gets saved. Music is muted on the page side. */
int web_audio_muted = 0;

EMSCRIPTEN_KEEPALIVE void web_set_muted(int muted) {
  web_audio_muted = muted;
}

/* Write settings to /prefs; the page then flushes /prefs to IndexedDB. */
EMSCRIPTEN_KEEPALIVE void web_save(void) {
  saveSettings();
}
