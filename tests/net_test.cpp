#include "gut/net.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do { if(!(x)) { std::cerr << "CHECK failed: " << #x << " at " << __LINE__ << "\n"; std::exit(1); } } while(false)
using namespace gut;
static bool reject(std::string_view s) {try{(void)decode_uri_component(s);return false;}catch(const std::invalid_argument&){return true;}}
static bool reject_api(std::string_view origin,std::string_view path){try{(void)resolve_trusted_api_url(origin,path);return false;}catch(const std::invalid_argument&){return true;}}
int main(){
 CHECK(percent_encode_rfc3986_component("a !'()*~/?") == "a%20%21%27%28%29%2A~%2F%3F");
 CHECK(encode_uri_component("a !'()*~/?") == "a%20!'()*~%2F%3F");
 CHECK(percent_encode_rfc3986_component("\xF0\x9F\x98\x80") == "%F0%9F%98%80");
 CHECK(decode_uri_component("%F0%9F%98%80") == "\xF0\x9F\x98\x80");
 CHECK(decode_uri_component("%2f%3F+%25") == "/?+%");
 for(const auto s:{"%","%4","%zz","%ff","%C0%AF","%ED%A0%80","%F4%90%80%80"})CHECK(reject(s));
 CHECK(join_url("https://api.example.com///",{"/v1/","/users/"}) == "https://api.example.com/v1/users");
 CHECK(join_url("///",{"///"})=="");CHECK(join_url("foo",{})=="foo");
 CHECK(is_url("https://example.com"));CHECK(!is_url("/relative")); CHECK(!is_url("http://"));
 CHECK(get_query_param("https://host/x?a=1&a=2&b=hi+there","a")=="1");
 CHECK(get_query_param("https://host/x?b=hi+there","b")=="hi there");
 CHECK(!get_query_param("https://host/x","missing").has_value());
 CHECK(set_query_param("https://host/x?a=1&a=2&b=3","a","hello world") == "https://host/x?a=hello+world&b=3");
 CHECK(set_query_param("https://host/x#fragment","a+b","c d") == "https://host/x?a%2Bb=c+d#fragment");
 CHECK(resolve_trusted_api_url("https://api.example.com","/v1/%2Fproject?x=1") == "https://api.example.com/v1/%2Fproject?x=1");
 for(const auto bad:{"../a","//example.com","/../a","/%2e%2e/a","/%252e%252e/a","/%5c/a","/%252f%2e%2e","/foo#evil","/a\\b"}) CHECK(reject_api("https://api.example.com",bad));
 CHECK(reject_api("http://api.example.com","/v1"));
 CHECK(reject_api("https://api.example.com/path","/v1"));
 CHECK(reject_api("https://user@api.example.com","/v1"));
}
