#include "gut/curl_http.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"luth_curl_integration_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
struct memory_tokens:gut::luth_auth_token_store {
 std::string token;
 std::string get()const override{return token;}
 void set(std::string v)override{token=std::move(v);}
 void clear()override{token.clear();}
};
int main(int argc,char** argv){
 if(argc!=2)return 2;
 const std::string origin=std::string("http://127.0.0.1:")+argv[1];
 auto tokens=std::make_shared<memory_tokens>();
 gut::luth_auth_client_config cfg;cfg.ecosystem_id="ultrst";cfg.application_id="app-1";
 cfg.origin=origin;cfg.luth_urls={origin};cfg.token_store=tokens;cfg.max_retries=0;
 auto curl=std::make_shared<gut::curl_http_adapter>();
 gut::luth_client client(cfg,[curl](const gut::luth_http_request& req){return (*curl)(req);});
 CHECK(!client.get_session().authenticated);
 CHECK(!client.sign_in_with_email({"luth@example.com","secret"}).empty());
 CHECK(tokens->get()=="luth_token");
 CHECK(client.get_session().authenticated);
 client.sign_out();
 CHECK(tokens->get().empty());
 CHECK(!client.get_session().authenticated);
}
