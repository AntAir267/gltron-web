# gltron-web

[GLtron](http://www.gltron.org/) 0.70 compiled to WebAssembly so it runs in a
browser: the original C game, its Lua menus and its 3D sound, not a remake.

- **Emscripten** compiles the C/C++ and Lua 5.0 interpreter to WebAssembly.
- **[gl4es](https://github.com/ptitSeb/gl4es)** (pinned in `build.sh`) translates
  GLtron's OpenGL 1.x renderer to WebGL.
- GLtron's own mixer does the effects (Doppler engines, crashes, recognizer).
  Music streams through an `<audio>` element, so long songs don't have to be
  downloaded before they play.

## Build

Needs the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
(found via `$EMSDK` or `~/emsdk`) and `ffmpeg` built with libopenmpt (to render
the `.it` soundtrack to MP3).

```bash
./build.sh          # release build into dist/
./build.sh debug    # -O0, assertions, source map
```

Serve `dist/` from any static web server. For testing, `tools/serve.py` serves it
with browser caching off, so a reload always gets the latest build:

```bash
python3 tools/serve.py 8765
```

The first build clones and compiles gl4es into `build/gl4es`.

To add skins (GLtron "artpacks": a folder of textures plus `artpack.lua`, shaped
like `gltron/art/default`) that live outside this repo:

```bash
EXTRA_ART="/path/to/myskin" ./build.sh
```

Players switch skins in Video → Artpack; a page can pick the starting one
(see `artpack` below).

To set the game up for your own site, point `SITE_JS` at a script. It becomes
`dist/site.js`, runs before `shell.js`, and can set any `window.GLTRON` option
below (like the starting `artpack`) plus `window.GLTRON_MUSIC` for your own
streamed playlist. A build with `SITE_JS` leaves out the stock soundtrack.

```bash
SITE_JS=/path/to/site.js EXTRA_ART="/path/to/myskin" ./build.sh
```

```js
// site.js
window.GLTRON = Object.assign({ artpack: 'myskin' }, window.GLTRON);
window.GLTRON_MUSIC = { base: 'https://cdn.example/music/', tracks: ['song.mp3'] };
```

Without `SITE_JS`, `site.js` lists the stock soundtrack, "Revenge of the Cats".

To build for your site and publish in one step, put your settings and your
site's publish command in `ship.local` (ignored by git), then run
`./build.sh ship`. It refuses to run with uncommitted changes, so what you
publish always matches a commit.

```bash
# ship.local (paths relative to this folder)
SITE_JS=../mysite/gltron/site.js
EXTRA_ART=../mysite/gltron/myskin
SHIP="python3 ../mysite/tools/ship-game.py gltron"
```

## Layout

| Path | What |
| --- | --- |
| `gltron/` | GLtron 0.70 source. Commit 1 is the pristine Debian orig tarball, commit 2 adds Debian's patches, later commits are the web port. `git log -p gltron/` shows every change. |
| `patches/debian/` | Debian's 0.70final-14 patches, for reference |
| `web/port/` | Web-only C/C++: SDL_sound stand-in, missing SDL 1.2 calls, streamed-music `SourceMusic`, page entry points |
| `web/index.html`, `web/shell.js` | The page: canvas sizing, loading bar, saved settings, music player, mute |
| `build.sh` | Builds gl4es, compiles everything, packages game data, renders music |

## Changes to GLtron

All web-only changes are behind `#ifdef __EMSCRIPTEN__` or `if(WEB)` in Lua, so
the tree still builds natively.

- **Main loop.** Browsers own the loop, so `SystemMainLoop` registers a
  per-frame callback instead of blocking, and `main.lua`'s
  `while` loop became `MainLoopReturned(status)`, which the frame callback
  calls when a screen exits.
- **Quit** from the credits screen calls `GLTRON.onQuit` instead of `exit()`.
- **Settings** are saved to `/prefs` (IndexedDB) on every screen change and
  when the tab is hidden.
- **Shadows** default to the translucent stencil version, since WebGL always
  has a stencil buffer.
- `nebu`'s `clamp()` is now `nebu_clamp()`: gl4es exports a `clamp()` too, and
  the linker bound gl4es's calls to GLtron's.
- `SourceSample` loads with `Sound_NewSampleFromFile` (Emscripten's SDL_RWops
  can't be read from C).
- Players 3 and 4 get web key codes for their default arrow/keypad keys:
  Emscripten's SDL numbers those keys differently from SDL 1.2.
- **Mouse look and click zoom are off.** GLtron's mouse look measures each move
  from a fixed point and warps the pointer back there. Pages can't move the
  pointer, so the offsets piled up and spun the camera. Holding a mouse button
  zoomed the camera, so every click to focus the page zoomed out. The web
  build ignores the mouse in the race; `C` cycles camera views instead.
- **No key repeat**, as in native GLtron (`SDL_EnableKeyRepeat(0, 0)`):
  Emscripten's SDL passes the browser's repeats on, so a held key turned the
  bike or cycled the camera over and over. `shell.js` drops them.
- **Keys:** player 1 defaults to the arrows (Up boosts, Delete/End glance) and
  player 3 to GLtron's old player 1 letters; settings saved before this get
  the new defaults once. Unbound arrows and A/S/E steer the first human
  player, and `C` cycles the camera. The in-race console scrolls with
  PageUp/PageDown.
- **Key names:** Emscripten's SDL only names a-z and 0-9, and numbers arrows,
  F-keys and the keypad above GLtron's joystick codes, so Configure Keys showed
  blanks and "unknown custom key". `web/port/sdl_compat.c` names every key.
- Touch controls queue key presses that are handled at the start of the next
  frame, exactly like real keys (`web_touch()` in `web/port/web.c`).
- **Game type "both"** (booster and wall acceleration) is the default;
  settings saved before this get it once.

One change applies to native builds too:

- **Boost meter:** a bar left of the minimap shows the booster tank in the
  player's trail color, with a notch at `booster_min` (the least it takes to
  start a boost). "wall" over the minimap lights up while wall acceleration
  is speeding the player up (`drawBoostMeter()` in
  `src/video/graphics_hud.c`).

## Embedding

`index.html` works on its own or in an `<iframe>`. Set options before
`shell.js` loads:

```html
<script>
  window.GLTRON = {
    muted: true,                       // start silent
    onQuit: function () { /* close the window */ },
    maxPixelRatio: 2,                  // cap render resolution on hi-DPI screens
    music: { base: 'https://cdn.example/music/', tracks: ['song.mp3'] },
    touch: 'auto',                     // on-screen controls: true, false or 'auto'
    artpack: 'myskin',                 // starting skin (?artpack= works too)
  };
</script>
<script src="shell.js"></script>
```

A track is a file name, or `{ file: 'album/01-song.mp3', title: 'Song' }` to show a
title in GLtron's Song menu (plain ASCII; the menu's font has nothing else).
With more than one track, the game moves on to the next one when a song ends.
`start: 'album/01-song.mp3'` opens every visit on that song, whatever the
player picked last time.

Then `GLTRON.setMuted(false)` turns sound on. Muting never changes the
player's saved Music/FX settings. Without `music`, the page uses
`window.GLTRON_MUSIC` from `site.js`. Track names appear in GLtron's Audio
menu without their extension.

Browsers only allow audio after the player interacts with the page; music
starts on the first key press or click.

## Controls

Player 1 turns with the Left / Right arrows, boosts with Up, and looks around
with `Delete` / `End`. GLtron's own player 1 letters work too while no human
player has them: `A` / `S` turn and `E` boosts (they belong to player 3 in the
web build). `C` (or `F10`) cycles the camera views, `Space` pauses, `Esc` opens
the menu, and `F1`–`F4` switch between single and split-screen layouts.
Players 2–4 are configurable in Game → Configure Keys.

The bar left of the minimap is your boost tank. It drains while you boost,
refills while you don't, and has to be above the notch to start a boost.
"wall" over the minimap lights up while riding close to another bike's
trail speeds you up.

On touch screens, on-screen controls appear by themselves and hide again when
a keyboard is used (add `?touch=1` or `?touch=0` to the URL to force them):

- **Racing:** tap the left or right half of the screen to turn, hold Boost
  to boost. The corner buttons change the camera view, pause, and open the
  menu.
- **Paused or round over:** tap anywhere to continue.
- **Menus:** the arrow pad moves and changes values, OK selects, Back goes up
  a menu (or back to the race from the top menu).

Turning uses the first human player's own key bindings, so it keeps working
after keys are rebound. Upright phones get the game at the top with the
controls below it; sideways phones put the steering hints in the side bars.

## Licenses

- GLtron: GPL-2.0-or-later, (C) 1999-2003 Andreas Umbach and contributors
  (`gltron/COPYING`). The web port is under the same license. If you host the
  build, link to this source.
- gl4es: MIT. Emscripten runtime: MIT / University of Illinois NCSA.
- Soundtrack "Revenge of the Cats" is (C) Peter Hajba (Skaven), distributed
  with GLtron.
