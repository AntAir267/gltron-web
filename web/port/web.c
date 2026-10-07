/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
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

/* On-screen touch controls. The numbers match TOUCH in web/shell.js. */
enum {
  TOUCH_UP, TOUCH_DOWN, TOUCH_LEFT, TOUCH_RIGHT, /* menu navigation */
  TOUCH_OK, TOUCH_BACK, TOUCH_PAUSE,
  TOUCH_TURN_LEFT, TOUCH_TURN_RIGHT, TOUCH_BOOST  /* the first human player */
};

extern void SystemQueueKey(int key, int state); /* nebu/base/system.c */

/* the key the first human player has bound to an action, so touch keeps
   working after the player rebinds keys */
static int human_key(const char *action) {
  int i, key = 0;
  for(i = 0; i < game->players; i++) {
    if(!game->player[i].ai->active) {
      scripting_RunFormat("return settings.keys[%d].%s", i + 1, action);
      scripting_GetIntegerResult(&key);
      return key;
    }
  }
  return 0;
}

EMSCRIPTEN_KEEPALIVE void web_touch(int control, int down) {
  int key;
  switch(control) {
  case TOUCH_UP: key = SYSTEM_KEY_UP; break;
  case TOUCH_DOWN: key = SYSTEM_KEY_DOWN; break;
  case TOUCH_LEFT: key = SYSTEM_KEY_LEFT; break;
  case TOUCH_RIGHT: key = SYSTEM_KEY_RIGHT; break;
  case TOUCH_OK: key = SYSTEM_KEY_RETURN; break;
  case TOUCH_BACK:
    if(current && !strcmp(current->name, "configure")) {
      /* this screen binds whatever key comes next and has no cancel;
         sending the key that's already bound leaves it unchanged */
      scripting_Run("return settings.keys[configure_player][configure_event]");
      scripting_GetIntegerResult(&key);
    } else {
      key = 27;
    }
    break;
  case TOUCH_PAUSE: key = ' '; break;
  case TOUCH_TURN_LEFT: key = human_key("left"); break;
  case TOUCH_TURN_RIGHT: key = human_key("right"); break;
  case TOUCH_BOOST: key = human_key("boost"); break;
  default: return;
  }
  if(key)
    SystemQueueKey(key, down ? SYSTEM_KEYSTATE_DOWN : SYSTEM_KEYSTATE_UP);
}

/* Write settings to /prefs; the page then flushes /prefs to IndexedDB. */
EMSCRIPTEN_KEEPALIVE void web_save(void) {
  saveSettings();
}
