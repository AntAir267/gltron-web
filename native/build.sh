#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron.
# Build GLtron for Linux (and the Steam Deck): gltron/ + native/port -> an AppImage.
#
#   native/build.sh            the game in build/native/AppDir
#   native/build.sh appimage   and dist-native/GLtron-<version>-x86_64.AppImage
#
# EXTRA_ART="dir ..." adds artpacks from outside this repo, and ARTPACK=name
# makes one the default.
#
# Needs, in native/third_party/ (not committed; symlinks are fine; README.md
# has the details):
#   zig-x86_64-linux-0.17.0/      builds everything for glibc 2.31, so the
#                                 result runs on Linux from 2020 on
#   sdl3-rel/                     SDL3, static, built with that Zig
#   zlib/ libpng/                 their sources
#   appimagetool-x86_64.AppImage, appimage-runtime-x86_64
# and ffmpeg with libopenmpt on the PATH (renders the .it soundtrack to MP3).
set -euo pipefail
export LC_ALL=C

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TP="$ROOT/native/third_party"
BUILD="$ROOT/build/native"
JOBS="${JOBS:-6}"
VERSION="$(cat "$ROOT/native/VERSION")"
Z="$(readlink -f "$TP/zig-x86_64-linux-0.17.0")/zig"
SDL="$(readlink -f "$TP/sdl3-rel")"
TARGET=x86_64-linux-gnu.2.31
LIBDIR=/usr/lib/x86_64-linux-gnu
SYSINC=(-idirafter /usr/include -idirafter /usr/include/x86_64-linux-gnu)
mkdir -p "$BUILD/obj" "$BUILD/gen"

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
  nebu/audio/{SoundSystem,Source,Source3D,SourceCopy,SourceEngine,SourceSample,SourceMusic}.cpp
)
ZLIB_C=(adler32 compress crc32 deflate gzclose gzlib gzread gzwrite infback inffast inflate inftrees trees uncompr zutil)
PNG_C=(png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil pngset pngtrans pngwio pngwrite pngwtran pngwutil)

cp "$TP/libpng/scripts/pnglibconf.h.prebuilt" "$BUILD/gen/pnglibconf.h"
INCLUDES=(
  -I"$ROOT/native/port/include" -I"$ROOT/web/port/include"
  -I"$G/src/include" -I"$G/nebu/include" -I"$G/nebu/include/base"
  -I"$G/nebu/include/scripting" -I"$G/lua/include" -I"$G/lua/src"
  -I"$BUILD/gen" -I"$TP/libpng" -I"$TP/zlib"
)
DEFINES=(
  -DGLTRON_NATIVE -DGLTRON_PORT -include "$ROOT/native/port/include/native.h"
  '-DDATA_DIR=native_data_dir()' '-DPREF_DIR=native_pref_dir()' '-DSNAP_DIR=native_pref_dir()'
  "-DSEPARATOR='/'" "-DVERSION=\"0.70-native-$VERSION\""
  -DPNG_INTEL_SSE_OPT=0 -DPNG_ARM_NEON_OPT=0
)
COMMON=(-target "$TARGET" -O2 -g -w -fno-strict-aliasing -fno-sanitize=undefined
        "${INCLUDES[@]}" "${DEFINES[@]}" "${SYSINC[@]}")

# one make job per source: rebuilt when it or a header it included changed
MK="$BUILD/Makefile"
{
  echo 'all: objs'
  echo '.SUFFIXES:'
  objs=""
  rule() { # obj src compiler flags...
    local obj="$1" src="$2" cc="$3"; shift 3
    objs+=" $obj"
    printf '%s: %s\n\t@mkdir -p $(dir $@)\n\t@%s %s -MMD -MP -c $< -o $@\n-include %s\n' \
      "$obj" "$src" "$cc" "$*" "${obj%.o}.d"
  }
  q() { printf '%q ' "$@"; }
  CF="$(q "${COMMON[@]}" -std=gnu89)"
  CXF="$(q "${COMMON[@]}" -std=gnu++98)"
  for f in "${SRC_C[@]}"; do rule "$BUILD/obj/gltron/${f%.*}.o" "$G/$f" "$Z cc" "$CF"; done
  for f in "${SRC_CXX[@]}"; do rule "$BUILD/obj/gltron/${f%.*}.o" "$G/$f" "$Z c++" "$CXF"; done
  rule "$BUILD/obj/web/sdl_sound.o" "$ROOT/web/port/sdl_sound.c" "$Z cc" "$(q "${COMMON[@]}" -std=gnu99)"
  for f in sdl1 pad; do
    rule "$BUILD/obj/native/$f.o" "$ROOT/native/port/$f.c" "$Z cc" \
      "$(q -target "$TARGET" -O2 -g -w -fno-sanitize=undefined -std=gnu11 -I"$SDL/include" \
           -I"$ROOT/native/port/include" "${SYSINC[@]}")"
  done
  rule "$BUILD/obj/native/native.o" "$ROOT/native/port/native.c" "$Z cc" "$(q "${COMMON[@]}" -std=gnu99 -D_GNU_SOURCE)"
  for f in "${ZLIB_C[@]}"; do rule "$BUILD/obj/zlib/$f.o" "$TP/zlib/$f.c" "$Z cc" "$(q -target "$TARGET" -O2 -w -fno-sanitize=undefined -D_LARGEFILE64_SOURCE=1 -I"$TP/zlib")"; done
  for f in "${PNG_C[@]}"; do rule "$BUILD/obj/png/$f.o" "$TP/libpng/$f.c" "$Z cc" "$(q -target "$TARGET" -O2 -w -fno-sanitize=undefined -DPNG_INTEL_SSE_OPT=0 -DPNG_ARM_NEON_OPT=0 -I"$BUILD/gen" -I"$TP/libpng" -I"$TP/zlib")"; done
  echo "objs:$objs"
  echo "OBJS :=$objs" > "$BUILD/objs.mk"
} > "$MK"
make -s -f "$MK" -j"$JOBS" objs
# shellcheck disable=SC2046
OBJS=$(sed 's/^OBJS :=//' "$BUILD/objs.mk")

# libGL only has to exist at link time: the player's own is used. A stand-in
# with the same exports keeps the build from depending on this machine's glibc.
if [ ! -f "$BUILD/stubs/libGL.so" ]; then
  mkdir -p "$BUILD/stubs"
  nm -D --defined-only "$LIBDIR/libGL.so.1" | awk '$2 ~ /^[TWi]$/ {print $3}' | sed 's/@.*//' | sort -u |
    grep -E '^[A-Za-z_][A-Za-z0-9_]*$' | sed 's/.*/void &(void) {}/' > "$BUILD/stubs/gl.c"
  "$Z" cc -target "$TARGET" -shared -fPIC -o "$BUILD/stubs/libGL.so" -Wl,-soname,libGL.so.1 "$BUILD/stubs/gl.c"
fi

A="$BUILD/AppDir"
mkdir -p "$A/usr/bin"
# shellcheck disable=SC2086
"$Z" c++ -target "$TARGET" -g -o "$A/usr/bin/gltron" $OBJS "$SDL/lib/libSDL3.a" \
  -L"$BUILD/stubs" -lGL -lm -ldl -lpthread -Wl,--allow-shlib-undefined

# ---- game data ----------------------------------------------------------------
D="$A/usr/share/gltron"
rm -rf "$D" && mkdir -p "$D/music"
cp -r "$G/scripts" "$G/data" "$G/art" "$D/"
for pack in ${EXTRA_ART:-}; do
  cp -r "$pack" "$D/art/$(basename "$pack")"
done
find "$D" -name 'Makefile*' -delete
# effects at the mixer's rate; the .ogg copies are unused
rm "$D"/data/*.ogg
for f in "$G"/data/*.wav; do
  ffmpeg -nostdin -loglevel error -y -i "$f" -ar 44100 -ac 2 -c:a pcm_s16le "$D/data/$(basename "$f")"
done
for f in "$G"/music/*.it; do
  mp3="$BUILD/music/$(basename "${f%.*}").mp3"
  if [ ! -f "$mp3" ] || [ "$f" -nt "$mp3" ]; then
    mkdir -p "$BUILD/music"
    ffmpeg -nostdin -loglevel error -y -i "$f" -ar 44100 -ac 2 -c:a libmp3lame -q:a 2 "$mp3"
  fi
  cp "$mp3" "$D/music/"
done
[ -z "${ARTPACK:-}" ] || printf '%s\n' "$ARTPACK" > "$D/default_artpack"

# ---- AppImage ------------------------------------------------------------------
[ "${1:-}" = appimage ] || { echo "built $A/usr/bin/gltron"; exit 0; }

L="$A/usr/share/licenses/gltron"
mkdir -p "$L"
cp "$G/COPYING" "$L/GLtron-COPYING"
cp "$(readlink -f "$TP/sdl3-rel")"/../SDL3-*/LICENSE.txt "$L/SDL3-LICENSE.txt"
cp "$TP/libpng/LICENSE" "$L/libpng-LICENSE"
cp "$TP/zlib/LICENSE" "$L/zlib-LICENSE"
ZIG="$(dirname "$Z")"
cp "$ZIG/LICENSE" "$L/Zig-LICENSE.txt"
for x in libcxx libcxxabi libunwind; do cp "$ZIG/lib/$x/LICENSE.TXT" "$L/LLVM-$x-LICENSE.txt"; done
sed -n '/^This software is available as a choice/,$p' "$ROOT/native/port/dr_mp3.h" > "$L/dr_mp3-LICENSE.txt"
cat > "$L/README.txt" <<EOT
GLtron 0.70 by Andreas Umbach and contributors, ported to the web and to Linux by
Anthony Airdo. Free software under the GNU GPL, version 2 or later (GLtron-COPYING).
Source code: https://github.com/AntAir267/gltron-web
EOT

cat > "$A/AppRun" <<'EOT'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
exec "$HERE/usr/bin/gltron" "$@"
EOT
chmod +x "$A/AppRun"
cat > "$A/gltron.desktop" <<'EOT'
[Desktop Entry]
Type=Application
Name=GLtron
Comment=Light cycle racing in 3D
Exec=gltron
Icon=gltron
Categories=Game;ArcadeGame;
StartupWMClass=gltron
EOT
cp "$ROOT/native/gltron-icon.png" "$A/gltron.png"
OUT="$ROOT/dist-native"
mkdir -p "$OUT"
ARCH=x86_64 "$TP/appimagetool-x86_64.AppImage" --no-appstream --runtime-file "$TP/appimage-runtime-x86_64" \
  "$A" "$OUT/GLtron-$VERSION-x86_64.AppImage" >/dev/null 2>"$BUILD/appimagetool.log" ||
  { tail -20 "$BUILD/appimagetool.log"; exit 1; }
ls -la "$OUT/GLtron-$VERSION-x86_64.AppImage"
