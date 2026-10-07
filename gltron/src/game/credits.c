/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#include "game/gltron.h"

static int coffset;

char *credits[] = {
  "",
  "   GLtron is written by " "\x03" "4" "Andreas Umbach",
  "",
  " Contributors:",
  " Programming: Darrell Walisser  Nicolas Deniaud",
  "              Todd Kirby  Andy Howe  Jon Atkins",
  " Art:         Nicolas Zimmermann",
  "              Charles Babbage       Tracy Brown",
  "              Tyler Esselstrom       Allen Bond",
  " Music:       Peter Hajba",
  " Sound:       Damon Law",
  " Web port:    Anthony Airdo",
  "",
  "Additional Thanks to:",
  "Xavier Bouchoux     Mike Field      Steve Baker",
  "Jean-Bruno Richard             Andrey Zahkhatov",
  "Bjonar Henden   Shaul Kedem    Jonas Gustavsson",
  "Mattias Engdegard     Ray Kelm     Thomas Flynn",
  "Martin Fierz    Joseph Valenzuela   Ryan Gordon",
  "Sam Lantinga                   Patrick McCarthy",
  "",
  "Thanks to my sponsors:",
  "  3dfx:              Voodoo5 5500 graphics card",
  "  Right Hemisphere:  3D exploration software",
#ifdef __EMSCRIPTEN__
  "",
  "Source: github.com/AntAir267/gltron-web",
#endif
  NULL
};

void mouseCredits (int buttons, int state, int x, int y)
{
	if ( state == SYSTEM_MOUSEPRESSED ) {
#ifdef __EMSCRIPTEN__
		SystemExitLoop(RETURN_QUIT);
#else
		SystemExit();
		exit(0);
#endif
	}
}

void keyCredits(int state, int k, int x, int y)
{
	if(state == SYSTEM_KEYSTATE_UP)
		return;
#ifdef __EMSCRIPTEN__
	SystemExitLoop(RETURN_QUIT);
#else
  SystemExit();
	exit(0);
#endif
}

void idleCredits(void) {
  scripting_RunGC();
  SystemPostRedisplay();
}

void drawCredits(void) {
  int time;
  int x, y;
  int h;
  int i;
  int n;
  float colors[][3] = { { 1.0, 0.0, 0.0 }, { 1.0, 1.0, 1.0 } };
  time = SystemGetElapsedTime() - coffset;

  glClearColor(.0, .0, .0, .0);
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  rasonly(gScreen);
  /* fit the whole list, but never larger than the original 24 rows */
  for(n = 0; credits[n] != NULL; n++);
  if(n < 23)
    n = 23;
  h = 2 * gScreen->vp_h / (3 * (n + 1));
  for(i = 0; i < time / 250; i++) {
    glColor3fv(colors[i % 2]);
    if(credits[i] == NULL) 
      break;
    x = 10;
    y = gScreen->vp_h - 3 * h * (i + 1) / 2;
    drawText(gameFtx, x, y, h, credits[i]);
  }
}
void displayCredits(void) {
  drawCredits();
  SystemSwapBuffers();
}

void initCredits(void) {
  coffset = SystemGetElapsedTime();
}

Callbacks creditsCallbacks = { 
  displayCredits, idleCredits, keyCredits, initCredits, 
  NULL, NULL, mouseCredits, NULL, "credits"
};
