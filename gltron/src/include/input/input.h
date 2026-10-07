/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#ifndef INPUT_H
#define INPUT_H

typedef struct {
  int mouse1;
  int mouse2;
  int mousex;
  int mousey;
  /* camera keys held down (web build: the arrow keys) */
  int cam_left;
  int cam_right;
  int cam_in;
  int cam_out;
} Input;


#define MOUSE_ORIG_X 100
#define MOUSE_ORIG_Y 100
#define MOUSE_CX 0.003f
#define MOUSE_CY 0.003f

void keyGame(int state, int key, int x, int y);
void Input_Idle();

extern int joy_threshold;
extern Input gInput;
#endif
