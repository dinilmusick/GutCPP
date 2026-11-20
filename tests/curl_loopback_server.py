"""Actual loopback HTTP integration harness for the optional libcurl adapter."""
import http.server
import json
import subprocess
import sys
import threading

class App(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):
        return
    def do_GET(self):
        if self.path=="/api/session":
            if self.headers.get("Authorization","")!="Bearer luth_token" or getattr(self.server,"revoked",False):
                self.send_response(401)
                self.end_headers()
                return
            data={"authenticated":True,"user":{"id":"u2","name":"Luth","email":"luth@example.com"},"session":{"id":"s2","userId":"u2","expiresAt":"later"}}
            b=json.dumps(data).encode()
            self.send_response(200)
            self.send_header("Content-Type","application/json")
            self.send_header("Content-Length",str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if self.path=="/test/login":
            self.send_response(200)
            self.send_header("Set-Cookie", "session=1; HttpOnly; Path=/; SameSite=Strict")
            self.end_headers()
            self.wfile.write(b"ok")
            return
        if self.path=="/api/auth/oidc/session":
            if "session=1" not in self.headers.get("Cookie", ""):
                self.send_response(401)
                self.end_headers()
                return
            output={"authenticated":True,"user":{"id":"u1","name":"Test","email":"test@example.com"},"session":{"id":"s1","userId":"u1"}}
            b=json.dumps(output).encode()
            self.send_response(200)
            self.send_header("Content-Type","application/json")
            self.send_header("Content-Length",str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        self.send_error(404)
    def do_POST(self):
        if self.path=="/api/registry/register":
            b=json.dumps({"registered":True,"ecosystemId":"ultrst","applicationId":"app-1","origin":"http://127.0.0.1","authenticationEnabled":True}).encode()
            self.send_response(200)
            self.send_header("Content-Type","application/json")
            self.send_header("Content-Length",str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if self.path=="/api/auth/sign-in/email":
            self.server.revoked=False
            b=json.dumps({"token":"luth_token","ok":True}).encode()
            self.send_response(200)
            self.send_header("Content-Type","application/json")
            self.send_header("Content-Length",str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if self.path=="/api/auth/sign-out":
            self.server.revoked=True
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"{}")
            return
        if self.path=="/api/auth/oidc/logout":
            n=int(self.headers.get("Content-Length","0"))
            body=self.rfile.read(n)
            if body!=b"{}" or "session=1" not in self.headers.get("Cookie",""):
                self.send_error(403)
                return
            self.send_response(200)
            self.send_header("Set-Cookie","session=; Max-Age=0; HttpOnly; Path=/")
            self.end_headers()
            self.wfile.write(b"{}")
            return
        self.send_error(404)

with http.server.HTTPServer(("127.0.0.1",0),App) as server:
    worker=threading.Thread(target=server.serve_forever,daemon=True)
    worker.start()
    try:
        p=subprocess.run([sys.argv[1],str(server.server_port)],capture_output=True,text=True,timeout=25)
        print(p.stdout,end="")
        print(p.stderr,end="",file=sys.stderr)
        sys.exit(p.returncode)
    finally:
        server.shutdown()
        worker.join(timeout=5)
