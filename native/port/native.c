/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* The native build's link to the game: install paths, window size, quitting,
   and what a controller does on each screen. */
#include "game/gltron.h"

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

extern void SystemQueueKey(int key, int state); /* nebu/base/system.c */

/* ---- paths ---- */
static void read_default(const char *dir, const char *file, const char *env) {
  char path[PATH_MAX], value[256];
  FILE *f;
  snprintf(path, sizeof(path), "%.*s/%s", PATH_MAX - 64, dir, file);
  f = fopen(path, "r");
  if(f) {
    if(fgets(value, sizeof(value), f)) {
      value[strcspn(value, "\r\n")] = 0;
      setenv(env, value, 0);
    }
    fclose(f);
  }
}

const char *native_data_dir(void) {
  static char dir[PATH_MAX];
  if(!dir[0]) {
    const char *env = getenv("GLTRON_DATA");
    if(env) {
      snprintf(dir, sizeof(dir), "%s", env);
    } else {
      /* usr/bin/gltron -> usr/share/gltron */
      char exe[PATH_MAX];
      ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
      char *slash;
      exe[n > 0 ? n : 0] = 0;
      slash = strrchr(exe, '/');
      if(slash) *slash = 0;
      snprintf(dir, sizeof(dir), "%.*s/../share/gltron", PATH_MAX - 32, exe);
    }
    /* a build's own skin and main song (see build.sh) */
    read_default(dir, "default_artpack", "GLTRON_ARTPACK");
    read_default(dir, "default_track", "GLTRON_TRACK");
    read_default(dir, "edition", "GLTRON_EDITION");
  }
  return dir;
}

static void make_dirs(char *path) {
  char *p;
  for(p = path + 1; *p; p++) {
    if(*p == '/') {
      *p = 0;
      mkdir(path, 0755);
      *p = '/';
    }
  }
  mkdir(path, 0755);
}

const char *native_pref_dir(void) {
  static char dir[PATH_MAX];
  if(!dir[0]) {
    /* an edition keeps its own settings, so its skin and songs stay its own */
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *edition;
    char name[64];
    native_data_dir();
    edition = getenv("GLTRON_EDITION");
    snprintf(name, sizeof(name), "gltron%s%.32s", edition ? "-" : "", edition ? edition : "");
    if(xdg && xdg[0] == '/')
      snprintf(dir, sizeof(dir), "%.*s/%s", PATH_MAX - 80, xdg, name);
    else
      snprintf(dir, sizeof(dir), "%.*s/.local/share/%s", PATH_MAX - 96, getenv("HOME"), name);
    make_dirs(dir);
  }
  return dir;
}

/* ---- window ---- */
void native_resize(int width, int height) {
  if(width < 64 || height < 48)
    return;
  setSettingi("width", width);
  setSettingi("height", height);
  initGameScreen();
  changeDisplay(-1);
}

/* the window closed, or Steam told the game to stop */
void native_quit(void) {
  saveSettings();
  SystemExit();
  exit(0);
}

/* ---- controllers ---- */

/* the keys of the n'th human player, so a controller keeps working after the
   keys are rebound, and goes through exactly what a key press does */
static int human(int n) {
  int i;
  for(i = 0; i < game->players; i++)
    if(game->player[i].ai->active == AI_HUMAN && n-- == 0)
      return i;
  return -1;
}

static int humans(void) {
  int i, count = 0;
  for(i = 0; i < game->players; i++)
    if(game->player[i].ai->active == AI_HUMAN)
      count++;
  return count;
}

static int bound_key(int player, const char *action) {
  int key = 0;
  if(player < 0)
    return 0;
  scripting_RunFormat("return settings.keys[%d].%s", player + 1, action);
  scripting_GetIntegerResult(&key);
  return key;
}

/* Which human each controller drives: every controller is the first human's,
   except in a game with several humans, where they take the next free human
   in the order they first press something. By first press and not by plug-in
   order because a handheld can show several devices for one controller, some
   of them silent. If two devices report the same input at nearly the same
   moment they're one controller seen twice, and stay together. */
static signed char pad_player[PAD_MAX];
static int next_player;
static int last_input = -1, last_player;
static Uint32 last_time;

static void reset_players(void) {
  int i;
  for(i = 0; i < PAD_MAX; i++)
    pad_player[i] = -1;
  next_player = 0;
}

static int player_for(int pad, int input) {
  int n = humans();
  if(n < 2)
    return human(0);
  if(pad_player[pad] < 0) {
    if(input == last_input && SDL_GetTicks() - last_time < 50) {
      pad_player[pad] = (signed char) last_player;
    } else {
      pad_player[pad] = (signed char) (next_player < n ? next_player++ : 0);
    }
  }
  last_input = input;
  last_player = pad_player[pad];
  last_time = SDL_GetTicks();
  return human(pad_player[pad]);
}

/* what an input means on the current screen; 0 means nothing */
static int key_for(int pad, int input) {
  const char *screen = current ? current->name : "";

  if(!strcmp(screen, "game")) {
    switch(input) {
    case PAD_START: return ' ';
    case PAD_BACK: return 27;
    case PAD_Y: return SYSTEM_KEY_F10;
    case PAD_LEFT: case PAD_STICK_LEFT: case PAD_LB:
      return bound_key(player_for(pad, input), "left");
    case PAD_RIGHT: case PAD_STICK_RIGHT: case PAD_RB:
      return bound_key(player_for(pad, input), "right");
    case PAD_A: case PAD_X: case PAD_RT: case PAD_LT: case PAD_UP:
      return bound_key(player_for(pad, input), "boost");
    case PAD_RSTICK_LEFT: return bound_key(player_for(pad, input), "glance_left");
    case PAD_RSTICK_RIGHT: return bound_key(player_for(pad, input), "glance_right");
    }
  } else if(!strcmp(screen, "pause")) {
    switch(input) {
    case PAD_A: case PAD_START: return ' ';
    case PAD_B: case PAD_BACK: return 27;
    case PAD_Y: return SYSTEM_KEY_F10;
    }
  } else if(!strcmp(screen, "gui")) {
    switch(input) {
    case PAD_UP: case PAD_STICK_UP: return SYSTEM_KEY_UP;
    case PAD_DOWN: case PAD_STICK_DOWN: return SYSTEM_KEY_DOWN;
    case PAD_LEFT: case PAD_STICK_LEFT: return SYSTEM_KEY_LEFT;
    case PAD_RIGHT: case PAD_STICK_RIGHT: return SYSTEM_KEY_RIGHT;
    case PAD_A: return SYSTEM_KEY_RETURN;
    case PAD_B: case PAD_BACK: case PAD_START: return 27;
    }
  } else if(!strcmp(screen, "configure")) {
    /* this screen binds whatever key comes next and has no cancel; sending
       the key that's already bound leaves it unchanged */
    if(input == PAD_B) {
      int key = 0;
      scripting_Run("return settings.keys[configure_player][configure_event]");
      scripting_GetIntegerResult(&key);
      return key;
    }
  } else if(!strcmp(screen, "credits")) {
    if(input == PAD_A || input == PAD_B || input == PAD_START)
      return SYSTEM_KEY_RETURN;
  }
  return 0;
}

/* The key each pad's input pressed, so its release goes to that key even if
   the screen changed in between. held_keys counts the pads holding each key:
   it's down for the first and up with the last. */
static int pressed[PAD_MAX][PAD_INPUTS];
static unsigned char held_keys[1024];
static int repeat_key;
static Uint32 repeat_at;

void native_pad_input(int pad, int input, int down) {
  static int started;
  int key;

  if(!started) {
    started = 1;
    reset_players();
  }
  if(current && !strcmp(current->name, "gui"))
    reset_players();

  if(down) {
    key = key_for(pad, input);
    if(getenv("GLTRON_TEST"))
      fprintf(stderr, "[pad] pad %d input %d on '%s' -> key %d\n", pad, input,
              current ? current->name : "", key);
    if(key <= 0 || key >= 1024)
      return;
    pressed[pad][input] = key;
    if(held_keys[key]++ == 0)
      SystemQueueKey(key, SYSTEM_KEYSTATE_DOWN);
    /* menu directions repeat while held, like a held key */
    if(!strcmp(current->name, "gui") &&
       (key == SYSTEM_KEY_UP || key == SYSTEM_KEY_DOWN ||
        key == SYSTEM_KEY_LEFT || key == SYSTEM_KEY_RIGHT)) {
      repeat_key = key;
      repeat_at = SDL_GetTicks() + 400;
    }
  } else {
    key = pressed[pad][input];
    if(!key)
      return;
    pressed[pad][input] = 0;
    if(key == repeat_key)
      repeat_key = 0;
    if(held_keys[key] && --held_keys[key] == 0)
      SystemQueueKey(key, SYSTEM_KEYSTATE_UP);
  }
}

void native_pad_tick(void) {
  if(!repeat_key)
    return;
  if(!current || strcmp(current->name, "gui")) {
    repeat_key = 0;
    return;
  }
  if(SDL_GetTicks() >= repeat_at) {
    SystemQueueKey(repeat_key, SYSTEM_KEYSTATE_DOWN);
    repeat_at = SDL_GetTicks() + 110;
  }
}

/* ---- scripted tests (pad.c) ---- */
static int shot_pending;
void native_test_shot(void) { shot_pending = 1; }
void native_test_key(int key, int down) {
  SystemQueueKey(key, down ? SYSTEM_KEYSTATE_DOWN : SYSTEM_KEYSTATE_UP);
}
void native_before_swap(void) {
  if(shot_pending) {
    shot_pending = 0;
    doPngScreenShot(gScreen);
  }
}
