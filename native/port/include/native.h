/* SPDX-License-Identifier: GPL-2.0-or-later
   Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron. */
/* Forced into every native compile (build.sh): the install paths are found at
   run time, because an AppImage is mounted somewhere different each start. */
#ifndef NATIVE_H
#define NATIVE_H

#ifdef __cplusplus
extern "C" {
#endif

const char *native_data_dir(void);
const char *native_pref_dir(void);

/* game side (native.c), called from the SDL side (sdl1.c, pad.c) */
void native_resize(int width, int height);
void native_quit(void);
void native_before_swap(void);
void native_pad_input(int pad, int input, int down);
void native_pad_tick(void);
void native_test_shot(void);
void native_test_key(int key, int down);

/* SDL side */
void native_pad_init(void);
void native_pad_event(const void *sdl_event);
void native_test_init(void);
void native_test_tick(void);

/* what a controller can do, whichever button or stick did it (pad.c, native.c) */
enum {
  PAD_A, PAD_B, PAD_X, PAD_Y, PAD_LB, PAD_RB, PAD_LT, PAD_RT, PAD_BACK, PAD_START,
  PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT,          /* D-pad */
  PAD_STICK_UP, PAD_STICK_DOWN, PAD_STICK_LEFT, PAD_STICK_RIGHT,
  PAD_RSTICK_LEFT, PAD_RSTICK_RIGHT,
  PAD_INPUTS
};
#define PAD_MAX 16

#ifdef __cplusplus
}
#endif
#endif
