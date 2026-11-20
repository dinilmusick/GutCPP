#include "gut/s3.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"s3_sigv4_test:"<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
template<typename Fn>bool rejects(Fn&& fn){try{fn();}catch(const std::exception&){return true;}return false;}
int main(){
 using namespace gut;
 CHECK(s3_detail::hex(s3_detail::sha256("abc"))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
 CHECK(s3_detail::hex(s3_detail::hmac("key","The quick brown fox jumps over the lazy dog"))=="f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
 CHECK(s3_detail::md5_base64("hello")=="XUFAKrxLKna5cZ2REBfFkg==");
 CHECK(s3_detail::encoded_key("folder/hello world.txt")=="folder/hello%20world.txt");
 CHECK(s3_detail::encoded_key("folder/")=="folder/");
 CHECK(s3_detail::encoded_key("a//b")=="a//b");
 CHECK(rejects([]{(void)s3_detail::encoded_key(std::string("\xFF",1));}));
 CHECK(s3_detail::encoded_query({{"prefix","a b"},{"list-type","2"}})=="list-type=2&prefix=a%20b");
 CHECK(rejects([]{(void)s3_detail::encoded_key("foo/../bar");}));
 CHECK(rejects([]{s3_detail::valid_bucket("in/valid");}));
 CHECK(rejects([]{s3_detail::valid_amz_date("20261120T190000");}));
 CHECK(s3_detail::xml_escape("a&b<c>\"'")=="a&amp;b&lt;c&gt;&quot;&apos;");
 std::vector<luth_http_request> requests;
 s3_credentials keys{"AKIDEXAMPLE","secret-example","token_example"};
 int status=200;
 luth_http_transport transport=[&](const luth_http_request& req){requests.push_back(req);return luth_http_response{status,"<ok/>",{{"etag","test"}}};};
 s3_client client(transport,[&]{return keys;},s3_options{},[]{return "20261120T141900Z";});
 CHECK(client.get_object("test-bucket","folder/hello world.txt")=="<ok/>");
 const auto& req=requests.back();
 CHECK(req.method=="GET" && req.url=="https://s3.amazonaws.com/test-bucket/folder/hello%20world.txt");
 CHECK(req.headers.at("host")=="s3.amazonaws.com");
 CHECK(req.headers.at("x-amz-date")=="20261120T141900Z");
 CHECK(req.headers.at("x-amz-security-token")=="token_example");
 CHECK(req.headers.at("x-amz-content-sha256")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 CHECK(req.headers.at("authorization").find("AWS4-HMAC-SHA256 Credential=AKIDEXAMPLE/20261120/us-east-1/s3/aws4_request,SignedHeaders=host;x-amz-content-sha256;x-amz-date;x-amz-security-token,Signature=")==0);
 CHECK(!req.credentials_include&&req.no_store);
 CHECK(client.list_objects_v2("test-bucket","a b",15,"p/")=="<ok/>");
 CHECK(requests.back().url=="https://s3.amazonaws.com/test-bucket?continuation-token=p%2F&list-type=2&max-keys=15&prefix=a%20b");
 CHECK(client.head_object("test-bucket","object").at("etag")=="test");
 CHECK(requests.back().method=="HEAD");
 CHECK(client.put_object("test-bucket","object",std::string("a\0b",3)).at("etag")=="test");
 CHECK(requests.back().method=="PUT"&&requests.back().body==std::string("a\0b",3));
 CHECK(client.copy_object("test-bucket","dest","test-bucket","source")=="<ok/>");
 CHECK(requests.back().headers.at("x-amz-copy-source")=="/test-bucket/source");
 client.delete_object("test-bucket","dest");CHECK(requests.back().method=="DELETE");
 CHECK(client.delete_objects("test-bucket",{"first","second"},true)=="<ok/>");
 CHECK(requests.back().url=="https://s3.amazonaws.com/test-bucket?delete=");
 CHECK(requests.back().headers.at("content-md5")==s3_detail::md5_base64(requests.back().body));
 CHECK(requests.back().body.find("<Quiet>true</Quiet>")!=std::string::npos);
 const auto count=requests.size();
 CHECK(rejects([&]{client.get_object("test-bucket","../secret");}));
 CHECK(rejects([&]{client.list_objects_v2("test-bucket","",0);}));
 CHECK(rejects([&]{client.delete_objects("test-bucket",{});}));
 CHECK(rejects([&]{client.get_object("bad/bucket","safe");}));
 CHECK(requests.size()==count);
 status=403;try{(void)client.get_object("test-bucket","safe");CHECK(false);}catch(const s3_error& e){CHECK(e.status()==403);}
 keys.secret_access_key="injected\nHost:evil";
 CHECK(rejects([&]{(void)client.get_object("test-bucket","safe");}));
 CHECK(rejects([&]{s3_client bad(transport,[]{return s3_credentials{"a","b",{}};},{"us-east-1","http://evil.example",false});}));
 return 0;
}
