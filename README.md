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

Serve `dist/` from any static web server, for example:

```bash
python3 -m http.server 8765 --directory dist
```

The first build clones and compiles gl4es into `build/gl4es`.

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
  };
</script>
<script src="shell.js"></script>
```

Then `GLTRON.setMuted(false)` turns sound on. Muting never changes the
player's saved Music/FX settings. Without `music`, the page uses
`music/tracks.js`, which `build.sh` writes. Track names appear in GLtron's Audio
menu without their extension.

Browsers only allow audio after the player interacts with the page; music
starts on the first key press or click.

## Controls

Player 1 turns with `A` / `S`, boosts with `D`, looks around with `Q` / `W`.
`Space` pauses, `Esc` opens the menu, `F10` changes the camera and
`F1`–`F4` switch between single and split-screen layouts.
Players 2–4 are configurable in Game → Configure Keys.

## Licenses

- GLtron: GPL-2.0-or-later, (C) 1999-2003 Andreas Umbach and contributors
  (`gltron/COPYING`). The web port is under the same license. If you host the
  build, link to this source.
- gl4es: MIT. Emscripten runtime: MIT / University of Illinois NCSA.
- Soundtrack "Revenge of the Cats" is (C) Peter Hajba (Skaven), distributed
  with GLtron.
