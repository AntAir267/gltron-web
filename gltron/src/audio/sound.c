/* Modified 2026-10 by Anthony Airdo for the web build (gltron-web). */
#include "game/gltron.h"
#include "filesystem/path.h"

#define NUM_GAME_FX 3

static char *game_fx_names[] = {

#if 1
  "game_engine.wav",
  "game_crash.wav",
  "game_recognizer.wav"
#else
  "game_engine.ogg",
  "game_crash.ogg",
  "game_recognizer.ogg"
#endif
};

void Sound_loadFX(void) {
  int i;
  char *path;


  for(i = 0; i < NUM_GAME_FX; i++) {
    path = getPath(PATH_DATA, game_fx_names[i]);
    if(path) {
      Audio_LoadSample(path, i);
      free(path);
    } else {
      fprintf(stderr, "[error] can't load sound fx file %s\n",
	     game_fx_names[i]);
      exit(1); // FIXME: handle missing fx somewhere else
    }
  }
}

void Sound_reloadTrack(void) {
  char *song;
  char *path;
	scripting_GetGlobal("settings", "current_track", NULL);
  scripting_GetStringResult(&song);
  fprintf(stderr, "[sound] loading song %s\n", song);
#ifdef __EMSCRIPTEN__
  path = getPossiblePath( PATH_MUSIC, song );
#else
  path = getPath( PATH_MUSIC, song );
#endif
  free(song);
  if(path == NULL) {
    fprintf(stderr, "[sound] can't find song...exiting\n");
    exit(1); // FIXME: handle missing song somewhere else
  }
  Sound_load(path);
  Sound_play();

  free(path);
}

void Sound_shutdown(void) {
  Audio_Quit();
}
  

void Sound_load(char *name) {
  Audio_LoadMusic(name);
}

void Sound_play(void) {
  Audio_SetMusicVolume(getSettingf("musicVolume"));
  Audio_PlayMusic();
  return;
}

void Sound_stop(void) {
  Audio_StopMusic();
}

void Sound_idle(void) {
  Audio_Idle();
}

void Sound_setMusicVolume(float volume) {
  if(volume > 1) volume = 1;
  if(volume < 0) volume = 0;
  Audio_SetMusicVolume(volume);
}

void Sound_setFxVolume(float volume) {
  if(volume > 1) volume = 1;
  if(volume < 0) volume = 0;
  Audio_SetFxVolume(volume);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* the page's track list, one file name per line */
EM_JS(char*, web_music_tracks, (void), {
  var names = (Module.music && Module.music.tracks) ? Module.music.tracks() : [];
  return stringToNewUTF8(names.join("\n"));
});

/* the track the page wants every visit to start on, or "" */
EM_JS(char*, web_music_start, (void), {
  return stringToNewUTF8((Module.music && Module.music.start && Module.music.start()) || "");
});

void Sound_initTracks(void) {
  char *names = web_music_tracks();
  char *name;
  int i = 1;

  for(name = strtok(names, "\n"); name != NULL; name = strtok(NULL, "\n")) {
    scripting_RunFormat("tracks[%d] = \"%s\"", i, name);
    i++;
  }
  free(names);
  if(i == 1) {
    /* no music: keep the Song menu working */
    scripting_Run("tracks[1] = \"none\"");
  }
  name = web_music_start();
  if(*name)
    scripting_RunFormat("settings.current_track = \"%s\"", name);
  free(name);
  scripting_Run("setupSoundTrack()");
}
#else
void Sound_initTracks(void) {
  const char *music_path;
  List *soundList;
  List *p;
  int i;

  music_path = getDirectory( PATH_MUSIC );
	soundList = readDirectoryContents(music_path, NULL);
  if(soundList->next == NULL) {
    fprintf(stderr, "[sound] no music files found...exiting\n");
    exit(1); // FIXME: handle missing songs somewhere else
  }
    
#ifdef GLTRON_NATIVE
  {
    /* in a fixed order, starting on a build's main song (default_track in
       native/port/native.c), like the site's player */
    int n = 0, a, b;
    List **all;
    const char *start = getenv("GLTRON_TRACK");
    for(p = soundList; p->next != NULL; p = p->next)
      n++;
    all = malloc(n * sizeof(List *));
    n = 0;
    for(p = soundList; p->next != NULL; p = p->next)
      all[n++] = p;
    for(a = 0; a < n; a++)
      for(b = a + 1; b < n; b++)
        if(strcmp((char *) all[a]->data, (char *) all[b]->data) > 0) {
          void *t = all[a]->data;
          all[a]->data = all[b]->data;
          all[b]->data = t;
        }
    free(all);
    if(start)
      scripting_RunFormat("settings.current_track = \"%s\"", start);
  }
#endif
  i = 1;
  for(p = soundList; p->next != NULL; p = p->next) {
    
    // bugfix: filter track list to readable files (and without directories)
    char *path = getPossiblePath( PATH_MUSIC, (char*)p->data );
  	if( path != NULL && fileExists( path ) ) {
    	scripting_RunFormat("tracks[%d] = \"%s\"", i, (char*) p->data);
        i++;
    	free( path );
    }
  }
  scripting_Run("setupSoundTrack()");
}
#endif

void Sound_setup(void) {
  printf("[sound] initializing sound\n");

  Audio_Init();
  Sound_loadFX();
  Audio_LoadPlayers();
  Sound_setFxVolume(getSettingf("fxVolume"));
  Sound_reloadTrack();
  Sound_setMusicVolume(getSettingf("musicVolume"));
  Audio_Start();
}
