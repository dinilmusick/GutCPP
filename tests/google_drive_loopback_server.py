"""Real loopback HTTP integration harness exercising Google Drive v3 wire behavior.
No OAuth account, tokens, or external network access is required.
"""
import http.server
import json
import subprocess
import sys
import threading
from urllib.parse import parse_qs, urlsplit

BINARY = b"\x00native\xff"

class Drive(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        return
    def respond(self, status, data=b"", content_type="application/json", cookie=None):
        if isinstance(data, (dict, list)):
            data = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        if cookie is not None:
            self.send_header("Set-Cookie", cookie)
        self.end_headers()
        self.wfile.write(data)
    def authorized(self):
        if self.headers.get("Authorization") != "Bearer integration_token" or self.headers.get("Cookie"):
            self.respond(403, {"error": "invalid bearer credentials or cookie leakage"})
            return False
        return True
    def data(self):
        length = int(self.headers.get("Content-Length", "0"))
        if length < 0 or length > 4096:
            raise ValueError("Unsafe body length")
        return self.rfile.read(length)
    def do_GET(self):
        parsed=urlsplit(self.path)
        if parsed.path=="/set-cookie":
            self.respond(200,b"ok",cookie="native_session=secret; Path=/; HttpOnly")
            return
        if parsed.path=="/check-cookie":
            self.respond(200 if self.headers.get("Cookie")=="native_session=secret" else 403,b"ok")
            return
        if not self.authorized(): return
        if parsed.path=="/drive/v3/files":
            qs=parse_qs(parsed.query)
            if qs.get("pageSize")!=["2"] or qs.get("q")!=["name contains 'Test'"]:
                self.respond(400,{"error":"bad list query"});return
            self.respond(200,{"files":[{"id":"file_1"}]})
        elif parsed.path=="/drive/v3/files/file_1" and parsed.query=="alt=media":
            self.respond(200,BINARY,"application/octet-stream")
        elif parsed.path=="/drive/v3/files/file_1/export" and parse_qs(parsed.query)=={"mimeType":["text/plain"]}:
            self.respond(200,b"exported","text/plain")
        elif parsed.path=="/drive/v3/files/file_1":
            self.respond(200,{"id":"file_1"})
        else:
            self.respond(404,{"error":"not found"})
    def do_POST(self):
        if not self.authorized():return
        parsed=urlsplit(self.path)
        body=self.data()
        if parsed.path=="/drive/v3/files" and json.loads(body)["name"]=="New Document":
            self.respond(200,{"id":"created"})
        elif parsed.path=="/upload/drive/v3/files" and parse_qs(parsed.query)=={"uploadType":["media"]} and body==BINARY:
            self.respond(200,{"id":"uploaded"})
        elif parsed.path=="/drive/v3/files/file_1/copy" and json.loads(body)["name"]=="Copied":
            self.respond(200,{"id":"copied"})
        else:
            self.respond(400,{"error":"invalid POST"})
    def do_PATCH(self):
        if not self.authorized():return
        parsed=urlsplit(self.path)
        body=self.data()
        if parsed.path=="/upload/drive/v3/files/file_1" and parse_qs(parsed.query)=={"uploadType":["media"]} and body==BINARY:
            self.respond(200,{"id":"updated"})
        elif parsed.path=="/drive/v3/files/file_1" and json.loads(body)["name"]=="Renamed":
            self.respond(200,{"id":"renamed"})
        else:
            self.respond(400,{"error":"invalid PATCH"})
    def do_DELETE(self):
        if not self.authorized():return
        if urlsplit(self.path).path=="/drive/v3/files/file_1":
            self.respond(204)
        else:
            self.respond(404)

with http.server.HTTPServer(("127.0.0.1",0),Drive) as server:
    thread=threading.Thread(target=server.serve_forever,daemon=True)
    thread.start()
    try:
        result=subprocess.run([sys.argv[1],str(server.server_port)],capture_output=True,text=True,timeout=30)
        print(result.stdout,end="");print(result.stderr,file=sys.stderr,end="")
        sys.exit(result.returncode)
    finally:
        server.shutdown();thread.join(timeout=5)
