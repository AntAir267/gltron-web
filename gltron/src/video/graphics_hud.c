/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#include "video/video.h"
#include "game/game.h"

void drawAI(Visual *d) {
  char ai[] = "computer player";

  rasonly(d);
  glColor3f(1.0, 1.0, 1.0);
  drawText(gameFtx, d->vp_w / 4, 10, d->vp_w / (2 * strlen(ai)), ai);
  /* glRasterPos2i(100, 0); */
}

void drawPause(Visual *display) {
  char pause[] = "Game is paused";
  char winner[] = "Player %d wins!";
  char nowinner[] = "No one wins!";
  char buf[100];
  char *message;
  static float d = 0;
  static float lt = 0;
  float delta;
  int now;

  now = SystemGetElapsedTime();
  delta = now - lt;
  lt = now;
  delta /= 500.0;
  d += delta;
  /* printf("%.5f\n", delta); */
  
  if (d > 2 * PI) { 
    d -= 2 * PI;
  }

  if ((game->pauseflag & PAUSE_GAME_FINISHED) && game->winner != -1) {
    if (game->winner >= -1) {

      float* player_color = gPlayerVisuals[game->winner].pColorAlpha;

      /* 
         make the 'Player wins' message oscillate between 
         white and the winning bike's color 
       */
      glColor3f((player_color[0] + ((sinf(d) + 1) / 2) * (1 - player_color[0])),
                (player_color[1] + ((sinf(d) + 1) / 2) * (1 - player_color[1])),
                (player_color[2] + ((sinf(d) + 1) / 2) * (1 - player_color[2]))); 

      message = buf;
      sprintf(message, winner, game->winner + 1);
    } else {
      glColor3d(1.0, (sin(d) + 1) / 2, (sin(d) + 1) / 2);
      message = nowinner;
    }
  } else {
    glColor3d(1.0, (sin(d) + 1) / 2, (sin(d) + 1) / 2);
    message = pause;
  }

  rasonly(gScreen);
  drawText(gameFtx, display->vp_w / 6, 20, 
	   display->vp_w / (6.0f / 4.0f * strlen(message)), message);
}

void drawScore(Player *p, Visual *d) {
  char tmp[10]; /* hey, they won't reach such a score */

  sprintf(tmp, "%d", p->data->score);
  rasonly(d);
  glColor4f(1.0, 1.0, 0.2f, 1.0);
  drawText(gameFtx, 5, 5, 32, tmp);
}

  
/* The boost meter: a vertical bar left of the minimap showing the player's
   booster tank in their trail color, with a notch at booster_min (the least
   it takes to start boosting; below it the bar is dim) and brighter while
   boosting. With wall acceleration on, "wall" over the minimap lights up
   while an enemy trail speeds the player up. */
void drawBoostMeter(Player *p, PlayerVisual *pV, Visual *d) {
  int booster = getSettingf("booster_on") == 1;
  int wall = getSettingf("wall_accel_on") == 1;
  float *c = pV->pColorAlpha;
  float mx, s, x, y, w, h, gap, label, max, level, notch, a, glow;
  int usable;

  if((!booster && !wall) || p->data->speed <= 0)
    return;

  /* the minimap is the square that fits a map_ratio-sized box, 20 pixels
     in from the corner (drawGame, draw2D) */
  if(gSettingsCache.map_ratio_w > 0) {
    float bw = d->vp_w * gSettingsCache.map_ratio_w;
    float bh = d->vp_h * gSettingsCache.map_ratio_h;
    s = bw < bh ? bw : bh;
    mx = 20 + (bw - s) / 2;
  } else {
    s = d->vp_h / 3.0f;
    mx = 20 + s * 0.2f;
  }
  w = s * 0.07f;
  gap = s * 0.03f;
  x = mx - gap - w;
  if(x < 4)
    x = 4;
  /* the bar's top lines up with the minimap's; its bottom stays clear of
     the score (drawScore: 32 pixels high, 5 in from the corner) */
  y = 20;
  if(gSettingsCache.show_scores && y < 5 + 32 + gap)
    y = 5 + 32 + gap;
  h = 20 + s - y;
  label = s * 0.055f;

  rasonly(d);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if(booster) {
    max = getSettingf("booster_max");
    level = p->data->booster / max;
    notch = getSettingf("booster_min") / max;
    usable = p->data->boost_enabled || p->data->booster > getSettingf("booster_min");
    a = usable ? 0.9f : 0.35f;
    glow = p->data->boost_enabled ? 0.45f : 0;

    glColor4f(0, 0, 0, 0.45f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glColor4f(c[0] + (1 - c[0]) * glow, c[1] + (1 - c[1]) * glow,
              c[2] + (1 - c[2]) * glow, a);
    glVertex2f(x, y); glVertex2f(x + w, y);
    glVertex2f(x + w, y + h * level); glVertex2f(x, y + h * level);
    glEnd();

    glColor4f(1, 1, 1, 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + w, y); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(x, y + h * notch); glVertex2f(x + w, y + h * notch);
    glEnd();

    glColor4f(1, 1, 1, usable ? 0.9f : 0.5f);
    drawText(gameFtx, x, 20 + s + gap, label, "boost");
  }
  if(wall) {
    /* on the label row, right-aligned to the minimap */
    glColor4f(1, 1, 1, p->data->wall_accel_active ? 1.0f : 0.3f);
    drawText(gameFtx, mx + s - 4 * label, 20 + s + gap, label, "wall");
  }
  glDisable(GL_BLEND);
}

void drawFPS(Visual *d) {
#define FPS_HSIZE 20
  /* draws FPS in upper left corner of Display d */
  static int fps_h[FPS_HSIZE];
  static int pos = -FPS_HSIZE;
  static int fps_min = 0;
  static int fps_avg = 0;

  char tmp[20];
  int diff;

  rasonly(d);
  diff = (game2->time.dt > 0) ? game2->time.dt : 1;

  if(pos < 0) {
    fps_avg = 1000 / diff;
    fps_min = 1000 / diff;
    fps_h[pos + FPS_HSIZE] = 1000 / diff;
    pos++;
  } else {
    fps_h[pos] = 1000 / diff;
    pos = (pos + 1) % FPS_HSIZE;
    if(pos % 10 == 0) {
      int i;
      int sum = 0;
      int min = 1000;
      for(i = 0; i < FPS_HSIZE; i++) {
	sum += fps_h[i];
	if(fps_h[i] < min)
	  min = fps_h[i];
      }
      fps_min = min;
      fps_avg = sum / FPS_HSIZE;
    }
  }

  sprintf(tmp, "average FPS: %d", fps_avg);
  glColor4f(1.0, 0.4f, 0.2f, 1.0);
  drawText(gameFtx, d->vp_w - 180, d->vp_h - 20, 10, tmp);
  sprintf(tmp, "minimum FPS: %d", fps_min);
  drawText(gameFtx, d->vp_w - 180, d->vp_h - 35, 10, tmp);
  sprintf(tmp, "triangles: %d", polycount);
  drawText(gameFtx, d->vp_w - 180, d->vp_h - 50, 10, tmp);
}


void drawConsoleLines(char *line, int call) {
#define CONSOLE_SIZE 15
#define CONSOLE_X_OFF 20
  int size = CONSOLE_SIZE;
  int length;
  /* fprintf(stdout, "%s\n", line); */
  length = strlen(line);
  while(length * size > gScreen->vp_w / 2 - CONSOLE_X_OFF)
    size--;
    
  if(*line != 0) 
    drawText(gameFtx, CONSOLE_X_OFF, gScreen->vp_h - 20 * (call + 1),
	     size, line);
}

void drawConsole(Visual *d) {
  int lines;
  rasonly(d);
  glColor3f(1.0, 0.3f, 0.3f);
  
  if (gSettingsCache.softwareRendering) { 
    lines = 1;
  } else if (gScreen->vp_h < 600) {
    lines = 3;
  } else {
    lines = 5;
  }
  
  consoleDisplay(drawConsoleLines, lines);
}
