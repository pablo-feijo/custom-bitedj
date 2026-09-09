#!/usr/bin/env python3
import http.server
import socketserver
import subprocess
import threading
import sys

PORT = 8000

class AudioStreamHandler(http.server.BaseHTTPRequestHandler):
    def do_HEAD(self):
        if self.path.startswith('/stream.mp3'):
            self.send_response(200)
            self.send_header('Content-Type', 'audio/mpeg')
            self.send_header('Cache-Control', 'no-cache, no-store')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.send_header('Access-Control-Allow-Methods', 'GET, HEAD, OPTIONS')
            self.end_headers()
            return
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, HEAD, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        self.end_headers()

    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.end_headers()
            html = """<!DOCTYPE html>
<html>
<head>
    <title>BiteDJ Live Audio</title>
    <style>
        body { font-family: sans-serif; background: #121212; color: #fff; text-align: center; padding-top: 50px; }
        h1 { color: #a855f7; }
        audio { margin-top: 20px; width: 400px; }
        a { color: #38bdf8; text-decoration: none; }
    </style>
</head>
<body>
    <h1>BiteDJ Audio Stream</h1>
    <p>Live audio monitoring from Docker test instance</p>
    <audio controls autoplay src="/stream.mp3"></audio>
    <p style="margin-top: 30px;"><a href="http://localhost:6080/vnc.html?autoconnect=true&resize=scale&v=20260906" target="_blank">Open BiteDJ noVNC Web UI &rarr;</a></p>
</body>
</html>"""
            self.wfile.write(html.encode('utf-8'))
            return

        if self.path.startswith('/stream.mp3'):
            self.send_response(200)
            self.send_header('Content-Type', 'audio/mpeg')
            self.send_header('Cache-Control', 'no-cache, no-store')
            self.send_header('Connection', 'keep-alive')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            cmd = [
                'ffmpeg', '-nostdin', '-f', 'pulse', '-i', 'default',
                '-c:a', 'libmp3lame', '-b:a', '192k', '-f', 'mp3', 'pipe:1'
            ]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            try:
                while True:
                    chunk = proc.stdout.read(4096)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
            except Exception:
                pass
            finally:
                proc.terminate()
                proc.wait()
            return

        self.send_error(404)

class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True

if __name__ == '__main__':
    server = ThreadedHTTPServer(('0.0.0.0', PORT), AudioStreamHandler)
    print(f"Audio server running on http://0.0.0.0:{PORT}")
    server.serve_forever()
