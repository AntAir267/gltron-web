/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#include "game/gltron.h"
#include "game/timesystem.h"

void GameMode_Idle(void) {
	Sound_idle();
	Time_Idle();
	if(game2->time.dt == 0)
		return;
	Game_Idle();
	Video_Idle();
	Input_Idle();
	Scripting_Idle();
	SystemPostRedisplay();
}

void enterGame(void) { /* called when game mode is entered */
  updateSettingsCache();

  SystemHidePointer();
  SystemWarpPointer(MOUSE_ORIG_X, MOUSE_ORIG_Y);
  game2->time.offset = SystemGetElapsedTime() - game2->time.current;
  Audio_EnableEngine();
 
	// disable booster and cam offset
	{
		int i;
		for(i = 0; i < game->players; i++) {
			game->player[i].data->boost_enabled = 0;
			game->player[i].camera->movement[CAM_PHI_OFFSET] = 0;
		}
	}

  /* reset pause flag */
  if (game->pauseflag != PAUSE_GAME_FINISHED) {
    game->pauseflag = PAUSE_GAME_RUNNING;
  }
  
  /* fprintf(stderr, "init game\n"); */
}

void exitGame(void) {
  Audio_DisableEngine();
  /* fprintf(stderr, "exit game\n"); */
}

void gameMouseMotion(int x, int y) {
#ifdef __EMSCRIPTEN__
  /* This mouse look measures each move from (MOUSE_ORIG_X, MOUSE_ORIG_Y)
     and warps the pointer back there. A page can't move the pointer, so the
     offsets would pile up and spin the camera; C cycles camera views
     instead (keyGame). */
  return;
#endif
  if(x != MOUSE_ORIG_X || y != MOUSE_ORIG_Y) {
    gInput.mousex += x - MOUSE_ORIG_X;
    gInput.mousey += y - MOUSE_ORIG_Y;
    /* fprintf(stderr, "Mouse: dx: %d\tdy: %d\n", 
       x - MOUSE_ORIG_Y, y - MOUSE_ORIG_Y); */
    /* 
       cam_phi += - (x - MOUSE_ORIG_X) * MOUSE_CX;
       cam_chi += (y - MOUSE_ORIG_Y) * MOUSE_CY;
       if(cam_chi < CAM_CHI_MIN) cam_chi = CAM_CHI_MIN;
       if(cam_chi > CAM_CHI_MAX) cam_chi = CAM_CHI_MAX;
    */
    SystemWarpPointer(MOUSE_ORIG_X, MOUSE_ORIG_Y);
  }
}

void gameMouse(int buttons, int state, int x, int y) {
#ifdef __EMSCRIPTEN__
  /* Holding a mouse button zooms the camera. On a page, a click is how
     people focus the game, and each one zoomed out by a third, so the web
     build leaves the camera alone (like mouse look, see gameMouseMotion). */
  return;
#endif
  if(state == SYSTEM_MOUSEPRESSED) {
    if(buttons == SYSTEM_MOUSEBUTTON_LEFT) gInput.mouse1 = 1;
    if(buttons == SYSTEM_MOUSEBUTTON_RIGHT) gInput.mouse2 = 1;
  } else if(state == SYSTEM_MOUSERELEASED) {
    if(buttons == SYSTEM_MOUSEBUTTON_LEFT) gInput.mouse1 = 0;
    if(buttons == SYSTEM_MOUSEBUTTON_RIGHT) gInput.mouse2 = 0;
  }

  /*
  if(getSettingi("camType") == CAM_TYPE_MOUSE) 
    if(state == SYSTEM_MOUSEPRESSED) {
      if(buttons == SYSTEM_MOUSEBUTTON_LEFT) {
	cam_r -= CAM_DR;
	if(cam_r < CAM_R_MIN) cam_r = CAM_R_MIN;
      } else if(buttons == SYSTEM_MOUSEBUTTON_RIGHT) {
	cam_r += CAM_DR;
	if(cam_r > CAM_R_MAX) cam_r = CAM_R_MAX;
      }
    }
  */
  /* fprintf(stderr, "new cam_r: %.2f\n", cam_r); */
}

Callbacks gameCallbacks = { 
  displayGame, GameMode_Idle, keyGame, enterGame, exitGame, initGLGame, gameMouse, gameMouseMotion, "game"
};
