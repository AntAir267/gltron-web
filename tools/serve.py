#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2026 Anthony Airdo. Part of gltron-web, a web port of GLtron.
"""Serve dist/ for local testing, with browser caching off so every reload
gets the latest build.  usage: python3 tools/serve.py [port]"""
import functools
import http.server
import os
import sys


class NoCache(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()


port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
dist = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'dist')
handler = functools.partial(NoCache, directory=dist)
print(f'serving {dist} on port {port} (no caching)')
http.server.ThreadingHTTPServer(('', port), handler).serve_forever()
