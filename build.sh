#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron.
# Build GLtron for the web: gltron/ + gl4es -> dist/
#
#   ./build.sh          release build
#   ./build.sh debug    -O0, assertions, source maps
#   ./build.sh ship     release build with your site's settings, then your
#                       site's ship command, both from ship.local (not in git):
#                         SITE_JS=../mysite/gltron/site.js
#                         EXTRA_ART=../mysite/gltron/myskin
#                         SHIP="python3 ../mysite/tools/ship-game.py gltron"
#                       (paths relative to this folder)
#
# EXTRA_ART="dir ..." adds artpacks (folders shaped like gltron/art/default)
# that live outside this repo, e.g. a site's own skin.
# SITE_JS=file.js configures the game for a site: it runs before shell.js and
# can set window.GLTRON options (artpack, music, ...) and/or GLTRON_MUSIC. A
# build with it leaves out the stock soundtrack.
#
# Needs the Emscripten SDK (EMSDK env var, or ~/emsdk) and ffmpeg (with
# libopenmpt, for the .it soundtrack).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
MODE="${1:-release}"
SHIP=""
if [ "$MODE" = ship ]; then
  [ -f "$ROOT/ship.local" ] || { echo "ship: no ship.local (see the top of build.sh)" >&2; exit 1; }
  # a site's build ships a commit, so check for uncommitted changes before building
  if [ -n "$(git -C "$ROOT" status --porcelain)" ]; then
    echo "ship: commit your changes first" >&2; exit 1
  fi
  # shellcheck disable=SC1091
  source "$ROOT/ship.local"
  [ -n "$SHIP" ] || { echo "ship: ship.local doesn't set SHIP" >&2; exit 1; }
  abs() { local p out=""; for p in $1; do out+="$(cd "$ROOT" && realpath "$p") "; done; echo "${out% }"; }
  [ -z "${SITE_JS:-}" ] || SITE_JS="$(abs "$SITE_JS")"
  [ -z "${EXTRA_ART:-}" ] || EXTRA_ART="$(abs "$EXTRA_ART")"
  MODE=release
fi
BUILD="$ROOT/build/$MODE"
DIST="$ROOT/dist"

# gl4es translates GLtron's OpenGL 1.x calls to WebGL. Pinned for reproducible builds.
GL4ES_REPO=https://github.com/ptitSeb/gl4es.git
GL4ES_REV=ec16bedd8819c475326f4f1a3063772c6d986e06

if ! command -v emcc >/dev/null; then
  # shellcheck disable=SC1091
  source "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" >/dev/null 2>&1
fi

mkdir -p "$BUILD" "$DIST"

# ---- gl4es ----------------------------------------------------------------
GL4ES="$ROOT/build/gl4es"
if [ ! -f "$GL4ES/lib/libGL.a" ]; then
  if [ ! -d "$GL4ES/.git" ]; then
    git clone -q "$GL4ES_REPO" "$GL4ES"
  fi
  git -C "$GL4ES" fetch -q --depth 1 origin "$GL4ES_REV" 2>/dev/null || true
  git -C "$GL4ES" checkout -q "$GL4ES_REV"
  (cd "$GL4ES" && mkdir -p build-em && cd build-em &&
    emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DNOX11=ON -DNOEGL=ON -DSTATICLIB=ON >/dev/null &&
    emmake make -j"$(nproc)" >/dev/null 2>&1)
fi

# ---- GLtron sources ---------------------------------------------------------
G="$ROOT/gltron"
SRC_C=(
  src/gltron.c
  src/base/util.c
  src/input/input.c
  src/game/{camera,computer,computer_utilities,credits,engine,event,game,globals,gui,init,init_sdl,menu,pause,timedemo,switchCallbacks,scripting_interface}.c
  src/audio/sound.c
  src/configuration/settings.c
  src/filesystem/{path,dirsetup}.c
  src/video/{artpack,explosion,fonts,fonttex,gamegraphics,graphics_fx,graphics_hud,graphics_lights,graphics_utility,graphics_world,load_texture,material,model,recognizer,screenshot,skybox,texture,trail,trail_geometry,trail_render,video,visuals_2d}.c
  nebu/base/{geom,vector,matrix,random,util,system}.c
  nebu/input/{system_keynames,input_system}.c
  nebu/scripting/scripting.c
  nebu/filesystem/{filesystem,file_io,directory,findpath}.c
  nebu/video/{console,pixels,png_texture,video_system}.c
  lua/src/{lapi,lcode,ldebug,ldo,lfunc,lgc,llex,lmem,lobject,lparser,lstate,lstring,ltable,ltests,ltm,lundump,lvm,lzio}.c
  lua/src/lib/{lauxlib,lbaselib,ldblib,liolib,lmathlib,lstrlib}.c
)
SRC_CXX=(
  src/audio/sound_glue.cpp
  nebu/audio/{SoundSystem,Source,Source3D,SourceCopy,SourceEngine,SourceSample}.cpp
)
WEB_C=(
  web/port/web.c
  web/port/sdl_sound.c
  web/port/sdl_compat.c
)
WEB_CXX=(
  web/port/SourceMusic.cpp
)

INCLUDES=(
  -I"$GL4ES/include"          # first, so GL/gl.h is gl4es's
  -I"$ROOT/web/port/include"
  -I"$G/src/include" -I"$G/nebu/include" -I"$G/nebu/include/base"
  -I"$G/nebu/include/scripting" -I"$G/lua/include" -I"$G/lua/src"
)
DEFINES=(
  -DDATA_DIR='"/gltron"' -DPREF_DIR='"/prefs"' -DSNAP_DIR='"/prefs"'
  -DSEPARATOR="'/'" -DVERSION='"0.70-web"'
)
PORTS=(-sUSE_SDL=1 -sUSE_LIBPNG=1 -sUSE_ZLIB=1)

if [ "$MODE" = debug ]; then
  OPT=(-O0 -g)
  # ASSERTIONS=2 aborts on the fractional mouse coordinates SDL 1 reports
  # for a CSS-scaled canvas; release builds truncate them, which is fine
  LINK_OPT=(-sASSERTIONS=1 -gsource-map)
else
  OPT=(-O2)
  LINK_OPT=()
fi
COMMON=("${OPT[@]}" "${INCLUDES[@]}" "${DEFINES[@]}" "${PORTS[@]}" -w -fno-strict-aliasing)
CFLAGS=("${COMMON[@]}" -std=gnu89)
CXXFLAGS=("${COMMON[@]}" -std=gnu++98)

objs=()
# rebuild an object when its source or any header it included changed
stale() { # src obj dep
  [ -f "$2" ] && [ -f "$3" ] || return 0
  local f
  for f in "$1" $(sed -e 's/\\$//' -e 's/^[^:]*://' "$3"); do
    [ "$f" -nt "$2" ] && return 0
  done
  return 1
}
compile() { # src obj
  local src="$1" obj="$2" dep="${2%.o}.d"
  if stale "$src" "$obj" "$dep"; then
    mkdir -p "$(dirname "$obj")"
    case "$src" in
      *.cpp) em++ "${CXXFLAGS[@]}" -MMD -MF "$dep" -c "$src" -o "$obj" ;;
      *) emcc "${CFLAGS[@]}" -MMD -MF "$dep" -c "$src" -o "$obj" ;;
    esac
  fi
}
pids=()
for f in "${SRC_C[@]}" "${SRC_CXX[@]}"; do
  obj="$BUILD/obj/gltron/${f%.*}.o"; objs+=("$obj")
  compile "$G/$f" "$obj" & pids+=($!)
done
for f in "${WEB_C[@]}" "${WEB_CXX[@]}"; do
  obj="$BUILD/obj/${f%.*}.o"; objs+=("$obj")
  compile "$ROOT/$f" "$obj" & pids+=($!)
done
fail=0
for p in "${pids[@]}"; do wait "$p" || fail=1; done
[ $fail = 0 ] || { echo "compile failed" >&2; exit 1; }

# ---- game data ---------------------------------------------------------------
DATA="$BUILD/data/gltron"
rm -rf "$BUILD/data" && mkdir -p "$DATA"
cp -r "$G/scripts" "$G/data" "$G/art" "$DATA/"
for pack in ${EXTRA_ART:-}; do
  cp -r "$pack" "$DATA/art/$(basename "$pack")"
done
find "$DATA" -name 'Makefile*' -delete
# effects: the mixer runs at 22050 Hz, 16-bit stereo; the .ogg copies are unused
rm "$DATA"/data/*.ogg
for f in "$G"/data/*.wav; do
  ffmpeg -nostdin -loglevel error -y -i "$f" -ar 22050 -ac 2 -c:a pcm_s16le \
    "$DATA/data/$(basename "$f")"
done

# site.js: the site's own settings, or the stock soundtrack. Music is
# streamed by the page, not packaged.
rm -rf "$DIST/music"
if [ -n "${SITE_JS:-}" ]; then
  cp "$SITE_JS" "$DIST/site.js"
else
mkdir -p "$DIST/music"
tracks=()
for f in "$G"/music/*.it; do
  mp3="$DIST/music/$(basename "${f%.*}").mp3"
  if [ ! -f "$mp3" ] || [ "$f" -nt "$mp3" ]; then
    ffmpeg -nostdin -loglevel error -y -i "$f" -c:a libmp3lame -q:a 6 "$mp3"
  fi
  tracks+=("\"$(basename "$mp3")\"")
done
printf '// the stock soundtrack (build.sh)\nwindow.GLTRON_MUSIC = { base: "music/", tracks: [%s] };\n' \
  "$(IFS=,; echo "${tracks[*]}")" > "$DIST/site.js"
fi

# ---- link --------------------------------------------------------------------
rm -f "$DIST"/gltron.*
em++ "${OPT[@]}" "${LINK_OPT[@]}" "${PORTS[@]}" "${objs[@]}" "$GL4ES/lib/libGL.a" \
  -sFULL_ES2=1 -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 \
  -sEXPORTED_RUNTIME_METHODS=callMain,FS,ENV \
  -lidbfs.js \
  --preload-file "$BUILD/data/gltron@/gltron" \
  -o "$DIST/gltron.js"

cp "$ROOT/web/index.html" "$ROOT/web/shell.js" "$DIST/"
echo "built $DIST ($(du -sh "$DIST" | cut -f1))"

if [ -n "$SHIP" ]; then
  (cd "$ROOT" && bash -c "$SHIP")
fi
