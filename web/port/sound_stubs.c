/* Silent audio backend used until the Web Audio backend is in place. */
#include "audio/audio.h"
#include "Nebu_scripting.h"

void Sound_loadFX(void) {}
void Sound_init(void) {}
void Sound_shutdown(void) {}
void Sound_load(char *name) {}
void Sound_play(void) {}
void Sound_stop(void) {}
void Sound_idle(void) {}
void Sound_setMusicVolume(float volume) {}
void Sound_setFxVolume(float volume) {}
void Sound_reloadTrack(void) {}
void Sound_initTracks(void) {
  scripting_Run("tracks[1] = \"none\"");
  scripting_Run("setupSoundTrack()");
}
void Sound_setup(void) {}

void Audio_EnableEngine(void) {}
void Audio_DisableEngine(void) {}
void Audio_Idle(void) {}
void Audio_CrashPlayer(int player) {}
void Audio_LoadPlayers(void) {}
void Audio_Init(void) {}
void Audio_Start(void) {}
void Audio_Quit(void) {}
void Audio_LoadSample(char *name, int number) {}
void Audio_LoadMusic(char *name) {}
void Audio_PlayMusic(void) {}
void Audio_StopMusic(void) {}
void Audio_SetMusicVolume(float volume) {}
void Audio_SetFxVolume(float volume) {}
void Audio_StartEngine(int player) {}
void Audio_StopEngine(int player) {}
