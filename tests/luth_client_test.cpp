#include "gut/luth_client.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"luth_client_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
struct token_storage:gut::luth_auth_token_store {
 std::string value;
 std::string get()const override{return value;}
 void set(std::string s)override{value=std::move(s);}
 void clear()override{value.clear();}
};
int main(){
 auto tokens=std::make_shared<token_storage>();
 gut::luth_auth_client_config cfg;cfg.ecosystem_id="ultrst";cfg.application_id="app-1";
 cfg.origin="https://app.example.com";cfg.luth_urls={"https://a.example.com","https://b.example.com"};
 cfg.auto_register=false;cfg.max_retries=0;cfg.token_store=tokens;
 std::vector<gut::luth_http_request> calls;
 bool signed_out=false;
 const auto transport=[&](const gut::luth_http_request& req)->gut::luth_http_response{
   calls.push_back(req);
   if(req.url.rfind("https://a.example.com",0)==0)return {503,"",{}};
   if(req.url.find("/api/session")!=std::string::npos){
     if(signed_out)return {401,"",{}};
     return {200,R"({"authenticated":true,"user":{"id":"user1","email":"x@y.z","name":"X"},"session":{"id":"session1","userId":"user1","expiresAt":"later"}})",{{"set-auth-token","fresh"}}};
   }
   if(req.url.find("/api/auth/sign-out")!=std::string::npos){signed_out=true;return {200,"{}",{}};}
   if(req.url.find("/api/auth/sign-in/email")!=std::string::npos)return {200,R"({"token":"fromsignin","success":true})",{}};
   if(req.url.find("/api/auth/sign-in/social")!=std::string::npos)return {200,R"({"url":"https://auth.example.com/oauth"})",{}};
   return {404,R"({"code":"NOT_FOUND"})",{}};
 };
 gut::luth_client client(cfg,transport,[](int){});
 CHECK(client.get_auth_headers().empty());
 const auto login=client.sign_in_with_email({"x@y.z","pass"});CHECK(!login.empty()&&tokens->get()=="fromsignin");
 CHECK(calls.size()==2&&calls[0].url.find("https://a.example.com")==0&&calls[1].url.find("https://b.example.com")==0);
 CHECK(calls.back().headers.at("Content-Type")=="application/json");
 const auto session=client.get_session();CHECK(session.authenticated&&session.user->id=="user1"&&tokens->get()=="fresh");
 CHECK(client.get_auth_headers().at("Authorization")=="Bearer fresh");
 CHECK(client.sign_in_with_google().at("url").as<std::string>()=="https://auth.example.com/oauth");
 client.sign_out();CHECK(tokens->get().empty());CHECK(!client.get_session().authenticated);
 CHECK(client.get_auth_headers().empty());
 // Registration is serialized and cached; the same caller does not register twice.
 auto reg_config=cfg;reg_config.auto_register=true;reg_config.luth_urls={"https://b.example.com"};reg_config.origin="https://preview.example.com";
 int registered_count=0;
 const auto registered_transport=[&](const gut::luth_http_request& req)->gut::luth_http_response{
   if(req.url.find("/api/registry/register")!=std::string::npos){
     ++registered_count;
     const auto payload=gut::parse_json(req.body).as<gut::json_value::object>();
     CHECK(payload.at("environment").as<std::string>()=="preview");
     return {200,R"({"registered":true,"authenticationEnabled":true,"ecosystemId":"ultrst","applicationId":"app-1","origin":"https://preview.example.com"})",{}};
   }
   if(req.url.find("/api/session")!=std::string::npos)return {200,R"({"authenticated":false})",{}};
   return {500,"{}",{}};
 };
 gut::luth_client automatic(reg_config,registered_transport,[](int){});
 CHECK(!automatic.get_session().authenticated);
 CHECK(!automatic.get_session().authenticated&&registered_count==1);
 bool bad=false;try {auto clone=cfg;clone.origin="http://evil.example.com";gut::luth_client invalid(clone,transport,[](int){});}catch(const gut::luth_auth_error&){bad=true;}CHECK(bad);
 for(const auto& host : {"localhost.evil.example", "127.0.0.1.attacker.test", "[::1].attacker.test"}) {
   bad=false;
   try {auto clone=cfg;clone.origin=std::string("http://")+host;
     gut::luth_client invalid(clone,transport,[](int){});
   } catch(const gut::luth_auth_error&) {bad=true;}
   CHECK(bad);
 }
 CHECK(gut::luth_detail::loopback_authority("localhost:3000"));
 CHECK(gut::luth_detail::loopback_authority("127.0.0.1:443"));
 CHECK(gut::luth_detail::loopback_authority("[::1]:8080"));
 CHECK(!gut::luth_detail::loopback_authority("localhost:0"));
 CHECK(!gut::luth_detail::loopback_authority("localhost:65536"));
 CHECK(!gut::luth_detail::loopback_authority("127.0.0.1x:80"));
 bad=false;try {auto clone=cfg;clone.token_store.reset();gut::luth_client invalid(clone,transport,[](int){});}catch(const gut::luth_auth_error&){bad=true;}CHECK(bad);
 bad=false;try{auto clone=cfg;clone.luth_urls={"http://a.example.com"};gut::luth_client invalid(clone,transport,[](int){});}catch(const gut::luth_auth_error&){bad=true;}CHECK(bad);
}
