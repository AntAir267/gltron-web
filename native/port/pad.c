/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* Controllers. Every controller works, whenever it is plugged in: pads[] has
   each one SDL reports, and each button or stick push goes to native_pad_input()
   as "this pad, this input" (native.c decides what it means on the current
   screen, and merges pads that drive the same player). */
#include <SDL3/SDL.h>
#include "native.h"

static struct {
  SDL_JoystickID id;
  SDL_Gamepad *gamepad;
  unsigned char held[PAD_INPUTS];
} pads[PAD_MAX];

#define STICK_ON 16000    /* half way: a turn is a deliberate flick */
#define STICK_OFF 9000    /* back near the middle before it can fire again */
#define TRIGGER_ON 12000
#define TRIGGER_OFF 6000

static int find_pad(SDL_JoystickID id) {
  int i;
  for(i = 0; i < PAD_MAX; i++)
    if(pads[i].gamepad && pads[i].id == id)
      return i;
  return -1;
}

static void input(int pad, int which, int down) {
  if(pads[pad].held[which] == down)
    return;
  pads[pad].held[which] = (unsigned char) down;
  native_pad_input(pad, which, down);
}

static void add_pad(SDL_JoystickID id) {
  int i;
  if(find_pad(id) >= 0)
    return;
  for(i = 0; i < PAD_MAX; i++) {
    if(!pads[i].gamepad) {
      pads[i].gamepad = SDL_OpenGamepad(id);
      if(pads[i].gamepad) {
        pads[i].id = id;
        SDL_memset(pads[i].held, 0, sizeof(pads[i].held));
        SDL_Log("[pad] %d: %s", i, SDL_GetGamepadName(pads[i].gamepad));
      }
      return;
    }
  }
}

static void remove_pad(SDL_JoystickID id) {
  int pad = find_pad(id), i;
  if(pad < 0)
    return;
  /* nothing stays held down on a player when a controller is pulled */
  for(i = 0; i < PAD_INPUTS; i++)
    input(pad, i, 0);
  SDL_CloseGamepad(pads[pad].gamepad);
  pads[pad].gamepad = NULL;
  SDL_Log("[pad] %d: gone", pad);
}

void native_pad_init(void) {
  int i, n = 0;
  SDL_JoystickID *ids = SDL_GetGamepads(&n);
  for(i = 0; i < n; i++)
    add_pad(ids[i]);
  SDL_free(ids);
}

static int button_input(int button) {
  switch(button) {
  case SDL_GAMEPAD_BUTTON_SOUTH: return PAD_A;
  case SDL_GAMEPAD_BUTTON_EAST: return PAD_B;
  case SDL_GAMEPAD_BUTTON_WEST: return PAD_X;
  case SDL_GAMEPAD_BUTTON_NORTH: return PAD_Y;
  case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return PAD_LB;
  case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return PAD_RB;
  case SDL_GAMEPAD_BUTTON_BACK: return PAD_BACK;
  case SDL_GAMEPAD_BUTTON_START: return PAD_START;
  case SDL_GAMEPAD_BUTTON_DPAD_UP: return PAD_UP;
  case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return PAD_DOWN;
  case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return PAD_LEFT;
  case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return PAD_RIGHT;
  }
  return -1;
}

/* a stick direction fires when pushed past half way, and only along whichever
   axis is pushed furthest, so a sloppy flick left doesn't also count as up */
static void stick_axis(int pad, int on_neg, int on_pos, int value, int other) {
  int mag = value < 0 ? -value : value;
  int omag = other < 0 ? -other : other;
  int neg = pads[pad].held[on_neg], pos = pads[pad].held[on_pos];
  if(mag < STICK_OFF) {
    neg = pos = 0;
  } else if(mag > STICK_ON && mag >= omag) {
    neg = value < 0;
    pos = value > 0;
  }
  input(pad, on_neg, neg);
  input(pad, on_pos, pos);
}

void native_pad_event(const void *event) {
  const SDL_Event *e = event;
  int pad, which;

  switch(e->type) {
  case SDL_EVENT_GAMEPAD_ADDED:
    add_pad(e->gdevice.which);
    break;
  case SDL_EVENT_GAMEPAD_REMOVED:
    remove_pad(e->gdevice.which);
    break;
  case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
  case SDL_EVENT_GAMEPAD_BUTTON_UP:
    pad = find_pad(e->gbutton.which);
    which = button_input(e->gbutton.button);
    if(pad >= 0 && which >= 0)
      input(pad, which, e->gbutton.down);
    break;
  case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    pad = find_pad(e->gaxis.which);
    if(pad < 0)
      break;
    switch(e->gaxis.axis) {
    case SDL_GAMEPAD_AXIS_LEFTX:
      stick_axis(pad, PAD_STICK_LEFT, PAD_STICK_RIGHT, e->gaxis.value,
                 SDL_GetGamepadAxis(pads[pad].gamepad, SDL_GAMEPAD_AXIS_LEFTY));
      break;
    case SDL_GAMEPAD_AXIS_LEFTY:
      stick_axis(pad, PAD_STICK_UP, PAD_STICK_DOWN, e->gaxis.value,
                 SDL_GetGamepadAxis(pads[pad].gamepad, SDL_GAMEPAD_AXIS_LEFTX));
      break;
    case SDL_GAMEPAD_AXIS_RIGHTX:
      stick_axis(pad, PAD_RSTICK_LEFT, PAD_RSTICK_RIGHT, e->gaxis.value, 0);
      break;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
      if(e->gaxis.value > TRIGGER_ON) input(pad, PAD_LT, 1);
      else if(e->gaxis.value < TRIGGER_OFF) input(pad, PAD_LT, 0);
      break;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
      if(e->gaxis.value > TRIGGER_ON) input(pad, PAD_RT, 1);
      else if(e->gaxis.value < TRIGGER_OFF) input(pad, PAD_RT, 0);
      break;
    }
    break;
  }
}

/* ---- scripted tests: GLTRON_TEST=file, one "<ms> <what>" per line ----
     plug N                a virtual controller N (SDL's own virtual gamepad, so
     unplug N              it goes through everything a real one does)
     btn N <a|b|x|y|lb|rb|back|start|up|down|left|right> <0|1>
     axis N <lx|ly|rx|lt|rt> <value>
     key <sdl1 key number> <0|1>
     shot                  save the next frame to the screenshot folder
     quit */
static char *script, *script_at;
static Uint64 script_start;
static SDL_JoystickID vpads[8];
static SDL_Joystick *vjoys[8];

void native_test_init(void) {
  const char *path = SDL_getenv("GLTRON_TEST");
  if(path) {
    script = SDL_LoadFile(path, NULL);
    script_at = script;
    script_start = SDL_GetTicks();
    if(!script) SDL_Log("[test] can't read %s", path);
  }
}

static int test_button(const char *n) {
  static const struct { const char *name; int button; } b[] = {
    { "a", SDL_GAMEPAD_BUTTON_SOUTH }, { "b", SDL_GAMEPAD_BUTTON_EAST },
    { "x", SDL_GAMEPAD_BUTTON_WEST }, { "y", SDL_GAMEPAD_BUTTON_NORTH },
    { "lb", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER }, { "rb", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER },
    { "back", SDL_GAMEPAD_BUTTON_BACK }, { "start", SDL_GAMEPAD_BUTTON_START },
    { "up", SDL_GAMEPAD_BUTTON_DPAD_UP }, { "down", SDL_GAMEPAD_BUTTON_DPAD_DOWN },
    { "left", SDL_GAMEPAD_BUTTON_DPAD_LEFT }, { "right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
  };
  size_t i;
  for(i = 0; i < SDL_arraysize(b); i++)
    if(!SDL_strcmp(b[i].name, n))
      return b[i].button;
  return 0;
}

static int test_axis(const char *n) {
  if(!SDL_strcmp(n, "lx")) return SDL_GAMEPAD_AXIS_LEFTX;
  if(!SDL_strcmp(n, "ly")) return SDL_GAMEPAD_AXIS_LEFTY;
  if(!SDL_strcmp(n, "rx")) return SDL_GAMEPAD_AXIS_RIGHTX;
  if(!SDL_strcmp(n, "lt")) return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
  return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
}

void native_test_tick(void) {
  int ms, n, v;
  char what[16], arg[16];

  while(script_at && SDL_sscanf(script_at, "%d %15s", &ms, what) == 2) {
    const char *rest, *nl;
    if(SDL_GetTicks() - script_start < (Uint64) ms)
      return;
    rest = SDL_strstr(script_at, what) + SDL_strlen(what);
    nl = SDL_strchr(script_at, '\n');
    SDL_Log("[test] %.*s", nl ? (int) (nl - script_at) : 40, script_at);
    if(!SDL_strcmp(what, "plug") && SDL_sscanf(rest, "%d", &n) == 1) {
      SDL_VirtualJoystickDesc d;
      SDL_INIT_INTERFACE(&d);
      d.type = SDL_JOYSTICK_TYPE_GAMEPAD;
      d.naxes = SDL_GAMEPAD_AXIS_COUNT;
      d.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
      d.name = "Test controller";
      vpads[n] = SDL_AttachVirtualJoystick(&d);
      vjoys[n] = SDL_OpenJoystick(vpads[n]);
    } else if(!SDL_strcmp(what, "unplug") && SDL_sscanf(rest, "%d", &n) == 1) {
      SDL_CloseJoystick(vjoys[n]);
      SDL_DetachVirtualJoystick(vpads[n]);
    } else if(!SDL_strcmp(what, "btn") && SDL_sscanf(rest, "%d %15s %d", &n, arg, &v) == 3) {
      SDL_SetJoystickVirtualButton(vjoys[n], test_button(arg), v != 0);
    } else if(!SDL_strcmp(what, "axis") && SDL_sscanf(rest, "%d %15s %d", &n, arg, &v) == 3) {
      SDL_SetJoystickVirtualAxis(vjoys[n], test_axis(arg), (Sint16) v);
    } else if(!SDL_strcmp(what, "key") && SDL_sscanf(rest, "%d %d", &n, &v) == 2) {
      native_test_key(n, v);
    } else if(!SDL_strcmp(what, "shot")) {
      native_test_shot();
    } else if(!SDL_strcmp(what, "quit")) {
      SDL_Event e;
      SDL_zero(e);
      e.type = SDL_EVENT_QUIT;
      SDL_PushEvent(&e);
    }
    script_at = nl ? (char *) nl + 1 : NULL;
  }
  script_at = NULL;
}
