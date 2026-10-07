/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#include "base/nebu_system.h"

#include "SDL.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "scripting/nebu_scripting.h"
#endif

Callbacks *current = 0;
static int return_code = -1;
static int redisplay = 0;

void SystemExit() {
  fprintf(stderr, "[system] shutting down SDL now\n");
  SDL_Quit();
  fprintf(stderr, "[system] exiting application\n");
}

unsigned int SystemGetElapsedTime() {
  /* fprintf(stderr, "%d\n", SDL_GetTicks()); */
  return SDL_GetTicks();
}

static void SystemPollEvents(void) {
  SDL_Event event;

    while(SDL_PollEvent(&event) && current) {
			switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_JOYAXISMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
				SystemHandleInput(&event);
				break;
			case SDL_QUIT:
				SystemExit();
				break;
			default:
				/* ignore event */
				break;
      }
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(void, web_screen_changed, (const char *name), {
  if (Module.onScreen) Module.onScreen(UTF8ToString(name));
});

/* Keys from the page's touch controls (see web/port/web.c). They're
   handled at the start of the next frame, like real key presses. */
#define KEY_QUEUE_SIZE 32
static int queued_keys[KEY_QUEUE_SIZE][2];
static int queued_count = 0;

void SystemQueueKey(int key, int state) {
  if(queued_count < KEY_QUEUE_SIZE) {
    queued_keys[queued_count][0] = key;
    queued_keys[queued_count][1] = state;
    queued_count++;
  }
}

static void SystemDispatchQueuedKeys(void) {
  int i, n = queued_count;
  queued_count = 0;
  for(i = 0; i < n; i++)
    if(current && current->keyboard)
      current->keyboard(queued_keys[i][1], queued_keys[i][0], 0, 0);
}

/* The browser owns the loop: it calls SystemFrame once per animation frame.
   When a callback set asks to leave the loop (SystemExitLoop), main.lua's
   MainLoopReturned() picks the next one, as its while loop does natively. */
static void SystemFrame(void) {
  SystemPollEvents();
  SystemDispatchQueuedKeys();
  if(!current)
    return;
  current->idle();
  if(redisplay && return_code == -1) {
    current->display();
    redisplay = 0;
  }
  if(return_code != -1) {
    int status = return_code;
    if(current->exit)
      (current->exit)();
    return_code = -1;
    scripting_RunFormat("MainLoopReturned(%d)", status);
  }
}

int SystemMainLoop() {
  static int started = 0;
  if(!started) {
    started = 1;
    emscripten_set_main_loop(SystemFrame, 0, 0);
  }
  return -1;
}
#else
int SystemMainLoop() {
	return_code = -1;
  while(return_code == -1) {
    SystemPollEvents();
    if(redisplay) {
      current->display();
      redisplay = 0;
    } else
      current->idle();
  }
	if(current->exit)
		(current->exit)();
	return return_code;
}
#endif
  
void SystemRegisterCallbacks(Callbacks *cb) {
  current = cb;
#ifdef __EMSCRIPTEN__
  web_screen_changed(cb->name);
#endif
}

void SystemExitLoop(int value) {
	return_code = value;
}

void SystemPostRedisplay() {
  redisplay = 1;
}
