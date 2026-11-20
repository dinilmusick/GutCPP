"""Independent Amazon S3 SigV4 verifier over a real ephemeral TCP server.
The wire protocol and HMAC are verified in Python, not using the C++ signer.
"""
import base64
import hashlib
import hmac
import http.server
import subprocess
import sys
import threading
from urllib.parse import urlsplit, parse_qs

BYTES=b"\x00native\xff"
ID="AKIDEXAMPLE"
SECRET="secret-example"
AMZ_DATE="20261120T141900Z"
REGION="us-east-1"
TOKEN="token_example"

def mac(key,data):
    return hmac.new(key,data.encode() if isinstance(data,str) else data,hashlib.sha256).digest()

class S3(http.server.BaseHTTPRequestHandler):
    def log_message(self,*args):
        return
    def response(self,status=200,data=b"",headers=None):
        self.send_response(status)
        for k,v in (headers or {}).items():self.send_header(k,v)
        self.send_header("Content-Length",str(len(data)))
        self.end_headers()
        if self.command!="HEAD":self.wfile.write(data)
    def payload(self):
        n=int(self.headers.get("Content-Length","0"))
        if n<0 or n>100000:raise ValueError("Unsafe body")
        return self.rfile.read(n)
    def verify(self,body):
        auth=self.headers.get("Authorization","")
        if not auth.startswith("AWS4-HMAC-SHA256 Credential="):return False
        details=auth.split(" ",1)[1].split(",")
        vals={part.split("=",1)[0]:part.split("=",1)[1] for part in details}
        credential=vals.get("Credential","")
        if credential!=f"{ID}/20261120/{REGION}/s3/aws4_request":return False
        date=self.headers.get("x-amz-date","")
        if date!=AMZ_DATE or self.headers.get("x-amz-security-token")!=TOKEN:return False
        if self.headers.get("Cookie"):return False
        payload_hash=hashlib.sha256(body).hexdigest()
        if self.headers.get("x-amz-content-sha256")!=payload_hash:return False
        signed=vals.get("SignedHeaders","").split(";")
        if not signed or signed!=sorted(signed):return False
        canonical_headers="".join(f"{name}:{self.headers.get(name,'')}\n" for name in signed)
        target=urlsplit(self.path)
        canonical=f"{self.command}\n{target.path}\n{target.query}\n{canonical_headers}\n{';'.join(signed)}\n{payload_hash}"
        scope=f"20261120/{REGION}/s3/aws4_request"
        string_to_sign=f"AWS4-HMAC-SHA256\n{date}\n{scope}\n{hashlib.sha256(canonical.encode()).hexdigest()}"
        date_key=mac(("AWS4"+SECRET).encode(),"20261120")
        region_key=mac(date_key,REGION)
        service_key=mac(region_key,"s3")
        signing_key=mac(service_key,"aws4_request")
        signature=mac(signing_key,string_to_sign).hex()
        return hmac.compare_digest(signature,vals.get("Signature",""))
    def dispatch(self):
        url=urlsplit(self.path)
        if url.path=="/set-cookie":
            self.response(200,b"ok",{"Set-Cookie":"session=secret; Path=/; HttpOnly"});return
        if url.path=="/check-cookie":
            self.response(200 if self.headers.get("Cookie")=="session=secret" else 403,b"ok");return
        body=self.payload()
        if not self.verify(body):self.response(403,b"invalid signature or cookie leak");return
        if self.command=="GET" and url.path=="/test-bucket/folder/hello%20world.txt":self.response(200,BYTES);return
        if self.command=="HEAD" and url.path=="/test-bucket/folder/hello%20world.txt":self.response(200,headers={"ETag":"\"testetag\""});return
        if self.command=="PUT" and url.path=="/test-bucket/folder/hello%20world.txt" and body==BYTES:
            self.response(200,headers={"ETag":"\"testetag\""});return
        if self.command=="GET" and url.path=="/test-bucket" and parse_qs(url.query)=={"list-type":["2"],"max-keys":["3"],"prefix":["prefix"]}:
            self.response(200,b"<ListBucketResult></ListBucketResult>");return
        if self.command=="PUT" and url.path=="/test-bucket/copy" and self.headers.get("x-amz-copy-source")=="/test-bucket/folder/hello%20world.txt":
            self.response(200,b"<CopyObjectResult/>");return
        if self.command=="DELETE" and url.path=="/test-bucket/copy":self.response(204);return
        if self.command=="POST" and url.path=="/test-bucket" and url.query=="delete=":
            md5=base64.b64encode(hashlib.md5(body).digest()).decode()
            if self.headers.get("Content-MD5")!=md5 or b"<Key>first</Key>" not in body or b"<Key>second</Key>" not in body:
                self.response(400,b"invalid delete xml");return
            self.response(200,b"<DeleteResult/>");return
        self.response(404,b"not found")
    def do_GET(self):self.dispatch()
    def do_HEAD(self):self.dispatch()
    def do_PUT(self):self.dispatch()
    def do_POST(self):self.dispatch()
    def do_DELETE(self):self.dispatch()

with http.server.HTTPServer(("127.0.0.1",0),S3) as server:
    runner=threading.Thread(target=server.serve_forever,daemon=True);runner.start()
    try:
        child=subprocess.run([sys.argv[1],str(server.server_port)],capture_output=True,text=True,timeout=30)
        print(child.stdout,end="");print(child.stderr,file=sys.stderr,end="")
        sys.exit(child.returncode)
    finally:
        server.shutdown();runner.join(timeout=5)
