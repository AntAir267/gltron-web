// Host-page glue for gltron.js: canvas sizing, loading status, saved
// settings, streamed music and the mute switch.
//
// Embedders can set these before loading this script:
//   window.GLTRON = {
//     onQuit() {},          // the player chose Quit and left the credits
//     muted: false,         // start with all sound off
//     maxPixelRatio: 2,     // cap on render resolution per CSS pixel
//     music: { base: 'music/', tracks: ['song.mp3'] },  // else music/tracks.js
//   }
// and afterwards call window.GLTRON.setMuted(bool).
(function () {
  'use strict';

  var opts = window.GLTRON = window.GLTRON || {};
  var canvas = document.getElementById('canvas');
  var statusEl = document.getElementById('status');
  var barEl = document.getElementById('bar');
  var loadingEl = document.getElementById('loading');
  var ready = false;
  var muted = !!opts.muted;

  function setStatus(text) {
    // Emscripten reports "Downloading data... (done/total)"
    var m = /\((\d+(?:\.\d+)?)\/(\d+)\)/.exec(text || '');
    if (barEl) barEl.style.width = m ? (100 * m[1] / m[2]).toFixed(1) + '%' : '';
    if (statusEl) statusEl.textContent = (text || '').replace(/\s*\(.*\)$/, '');
    if (loadingEl) loadingEl.hidden = !text;
  }

  // Render at the canvas's on-screen size (CSS keeps it 4:3), sharp on hi-DPI.
  function resize() {
    if (!ready) return;
    var ratio = Math.min(window.devicePixelRatio || 1, opts.maxPixelRatio || 2);
    var w = Math.round(canvas.clientWidth * ratio);
    var h = Math.round(canvas.clientHeight * ratio);
    Module._web_resize(w, h);
  }

  // Settings live in /prefs, backed by IndexedDB. GLtron writes them on
  // every screen change (then calls onSettingsSaved) and when the tab hides.
  var flushing = false, flushAgain = false;
  function flush() {
    if (flushing) { flushAgain = true; return; }
    flushing = true;
    FS.syncfs(false, function (err) {
      flushing = false;
      if (err) console.warn('[web] saving settings failed', err);
      if (flushAgain) { flushAgain = false; flush(); }
    });
  }
  function save() {
    if (!ready) return;
    Module._web_save();
    flush();
  }

  // Music: GLtron says what should play (web/port/SourceMusic.cpp), an
  // <audio> element streams it. Browsers only allow playback after the
  // player has interacted with the page, so retry on the first input.
  var music = (function () {
    var cfg = opts.music || window.GLTRON_MUSIC || { base: 'music/', tracks: [] };
    var audio = new Audio();
    audio.preload = 'none';
    var want = { name: '', playing: false, volume: 0.5, loop: true };
    var loaded = '';

    function apply() {
      if (want.name !== loaded) {
        loaded = want.name;
        if (loaded) {
          audio.src = cfg.base + encodeURIComponent(loaded);
        } else {
          audio.removeAttribute('src');
          audio.load();
        }
      }
      audio.loop = want.loop;
      audio.volume = Math.max(0, Math.min(1, want.volume));
      audio.muted = muted;
      if (want.playing && loaded && document.visibilityState !== 'hidden') {
        var p = audio.play();
        if (p) p.catch(function () { /* not allowed yet: retried on input */ });
      } else {
        audio.pause();
      }
    }

    ['keydown', 'mousedown', 'touchend'].forEach(function (type) {
      window.addEventListener(type, function () {
        if (want.playing && audio.paused) apply();
      }, true);
    });
    document.addEventListener('visibilitychange', apply);

    return {
      // names go into a Lua string literal, so keep them plain
      tracks: function () {
        return (cfg.tracks || []).filter(function (n) { return !/["\\\n]/.test(n); });
      },
      sync: function (name, playing, volume, loop) {
        want = { name: name, playing: playing, volume: volume, loop: loop };
        apply();
      },
      apply: apply,
    };
  })();

  opts.setMuted = function (value) {
    muted = !!value;
    if (ready) Module._web_set_muted(muted ? 1 : 0);
    music.apply();
  };

  window.Module = {
    canvas: canvas,
    music: music,
    print: function (t) { console.log(t); },
    printErr: function (t) { console.warn(t); },
    setStatus: setStatus,
    onSettingsSaved: flush,
    onQuit: function () {
      save();
      if (opts.onQuit) opts.onQuit();
    },
    preRun: [function () {
      FS.mkdir('/prefs');
      FS.mount(IDBFS, {}, '/prefs');
      Module.addRunDependency('prefs');
      FS.syncfs(true, function (err) {
        if (err) console.warn('[web] loading settings failed', err);
        Module.removeRunDependency('prefs');
      });
    }],
    postRun: [function () {
      ready = true;
      setStatus('');
      Module._web_set_muted(muted ? 1 : 0);
      // main() has already run: GLtron's window exists, so fit it to the page
      resize();
      canvas.focus();
    }],
  };

  window.addEventListener('error', function () {
    setStatus('Something went wrong. Reload to try again.');
  });

  if (window.ResizeObserver) {
    new ResizeObserver(resize).observe(canvas);
  } else {
    window.addEventListener('resize', resize);
  }
  document.addEventListener('visibilitychange', function () {
    if (document.visibilityState === 'hidden') save();
  });
  window.addEventListener('pagehide', save);
  canvas.addEventListener('mousedown', function () { canvas.focus(); });

  var script = document.createElement('script');
  script.src = 'gltron.js';
  script.async = true;
  document.body.appendChild(script);
})();
