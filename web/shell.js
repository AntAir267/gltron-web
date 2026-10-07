// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron.
// Host-page glue for gltron.js: canvas sizing, loading status, saved
// settings, streamed music and the mute switch.
//
// Embedders can set these before loading this script:
//   window.GLTRON = {
//     onQuit() {},          // the player chose Quit and left the credits
//     muted: false,         // start with all sound off
//     maxPixelRatio: 2,     // cap on render resolution per CSS pixel
//     music: { base: 'music/', tracks: ['song.mp3'] },  // else music/tracks.js
//                           // tracks can also be { file: 'dir/song.mp3', title: 'Song' }
//     touch: 'auto',        // on-screen controls: true, false or 'auto'
//                           // (?touch=1 / ?touch=0 in the URL also works)
//     artpack: 'default',   // starting skin, if build.sh packaged it
//   }                       // (?artpack=name in the URL also works)
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
    var base = cfg.base ? cfg.base.replace(/\/?$/, '/') : '';
    var audio = new Audio();
    audio.preload = 'none';
    var want = { name: '', playing: false, volume: 0.5, loop: true };
    var loaded = '';

    // GLtron knows each track by a name; its Song menu shows the name minus
    // the extension. Names go into a Lua string and a file path, so keep them
    // plain: no quotes, backslashes, slashes or line breaks.
    var files = {};   // name -> file, relative to base
    var names = [];
    (cfg.tracks || []).forEach(function (t) {
      var file = typeof t === 'string' ? t : t.file;
      var name = typeof t === 'string' ? t : (t.title || '').replace(/["\\/\n\r]+/g, ' ').trim() + '.mp3';
      if (!file || /["\\/\n\r]/.test(typeof t === 'string' ? t : '') || name === '.mp3') return;
      for (var n = 2, unique = name; files[unique]; n++) unique = name.replace(/\.mp3$/, ' (' + n + ').mp3');
      files[unique] = file;
      names.push(unique);
    });

    function apply() {
      if (want.name !== loaded) {
        loaded = want.name;
        if (files[loaded]) {
          audio.src = base + files[loaded].split('/').map(encodeURIComponent).join('/');
        } else {
          audio.removeAttribute('src');
          audio.load();
        }
      }
      // with several songs, play through them instead of repeating one
      audio.loop = want.loop && names.length < 2;
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
    audio.addEventListener('ended', function () {
      if (ready && names.length > 1) Module._web_next_track();
    });

    return {
      tracks: function () { return names.slice(); },
      sync: function (name, playing, volume, loop) {
        want = { name: name, playing: playing, volume: volume, loop: loop };
        apply();
      },
      apply: apply,
    };
  })();

  // On-screen controls for touch screens. Each control's data-touch number
  // is a TOUCH_* value in web/port/web.c; web_touch() turns it into the key
  // GLtron expects, so these behave exactly like key presses.
  var touch = (function () {
    var el = document.getElementById('touch');
    var param = new URLSearchParams(location.search).get('touch');
    var mode = param === '1' ? true : param === '0' ? false :
      (opts.touch === undefined ? 'auto' : opts.touch);
    var held = {};  // pointerId -> { node, control, timer }

    function show(on) {
      el.hidden = !on;
      document.body.classList.toggle('touch', on);
    }
    show(mode === true ||
      (mode === 'auto' && window.matchMedia && matchMedia('(pointer: coarse)').matches));
    if (mode === 'auto') {
      // follow what the player actually uses, e.g. on a 2-in-1 laptop
      window.addEventListener('pointerdown', function (e) {
        if (e.pointerType === 'touch') show(true);
      }, true);
      window.addEventListener('keydown', function (e) {
        if (e.isTrusted) show(false);
      }, true);
    }

    function send(control, down) {
      if (ready) Module._web_touch(control, down ? 1 : 0);
    }
    function release(id) {
      var h = held[id];
      if (!h) return;
      delete held[id];
      clearTimeout(h.timer);
      h.node.classList.remove('held');
      send(h.control, false);
    }
    // menu arrows repeat while held, like a held key
    function repeat(id, delay) {
      var h = held[id];
      if (!h) return;
      h.timer = setTimeout(function () {
        send(h.control, true);
        repeat(id, 110);
      }, delay);
    }

    el.addEventListener('pointerdown', function (e) {
      var node = e.target.closest('[data-touch]');
      if (!node) return;
      release(e.pointerId);
      // keep the release coming to us if the finger slides off the control
      try { node.setPointerCapture(e.pointerId); } catch (err) { /* not a live pointer */ }
      node.classList.add('held');
      held[e.pointerId] = { node: node, control: +node.dataset.touch, timer: 0 };
      send(held[e.pointerId].control, true);
      if (node.hasAttribute('data-repeat')) repeat(e.pointerId, 400);
    });
    ['pointerup', 'pointercancel', 'lostpointercapture'].forEach(function (type) {
      el.addEventListener(type, function (e) { release(e.pointerId); });
    });

    return {
      screen: function (name) {
        for (var id in held) release(id);
        el.querySelectorAll('[data-on]').forEach(function (node) {
          node.hidden = node.dataset.on.split(' ').indexOf(name) < 0;
        });
      },
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
    onScreen: touch.screen,
    onQuit: function () {
      save();
      if (opts.onQuit) opts.onQuit();
    },
    preRun: [function () {
      var artpack = new URLSearchParams(location.search).get('artpack') || opts.artpack;
      if (artpack) ENV.GLTRON_ARTPACK = artpack;
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
