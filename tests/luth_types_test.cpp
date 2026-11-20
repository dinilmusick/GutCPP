#include "gut/luth_types.hpp"
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"luth_types_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
struct local_tokens:gut::luth_auth_token_store {
 std::string token;
 std::string get()const override{return token;}
 void set(std::string input)override{token=std::move(input);}
 void clear()override{token.clear();}
};
int main(){
 std::cerr << "luth_types_test: entered main\n";
 gut::luth_auth_client_config config;config.ecosystem_id="ultrst";config.application_id="app-1";
 CHECK(gut::validate_luth_auth_client_config(config));
 std::cerr << "luth_types_test: config accepted\n";
 config.max_retries=-1;CHECK(!gut::validate_luth_auth_client_config(config));config.max_retries=3;
 config.application_id="https://host";CHECK(!gut::validate_luth_auth_client_config(config));
 std::cerr << "luth_types_test: config rejected\n";
 local_tokens store;gut::luth_auth_token_store& dynamic=store;
 dynamic.set("secret");CHECK(dynamic.get()=="secret");dynamic.clear();CHECK(dynamic.get().empty());
 std::cerr << "luth_types_test: token store verified\n";
 gut::luth_session_result result;CHECK(!result.authenticated&&!result.user&& !result.session);
 result.authenticated=true;result.user=gut::luth_user{"user-1","Name","a@example.org",std::nullopt,true};
 result.session=gut::luth_session{"session-1","user-1","2026-12-01",std::nullopt};
 CHECK(result.user->id==result.session->user_id);
 std::cerr << "luth_types_test: session verified\n";
 try{throw gut::luth_auth_error("NOT_AUTHENTICATED","Missing token",401);}catch(const gut::luth_auth_error& ex){
   CHECK(ex.code()=="NOT_AUTHENTICATED");CHECK(ex.status()==401);
   CHECK(std::string(ex.what())=="[NOT_AUTHENTICATED] Missing token");
 }
 std::cerr << "luth_types_test: exception verified\n";
}
