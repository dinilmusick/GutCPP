#include "gut/curl_http.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"curl_loopback_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
int main(int argc,char** argv){
 if(argc!=2)return 2;
 const std::string origin=std::string("http://127.0.0.1:")+argv[1];
 gut::curl_http_adapter curl;
 auto fetch=[&](const gut::application_auth_request& req){return curl.fetch_application(origin,req);};
 gut::application_auth_client client(fetch,[](const std::string&){});
 auto first=client.get_session();CHECK(!first.authenticated);
 gut::luth_http_request login;login.url=origin+"/test/login";
 auto auth=curl(login);CHECK(auth.status==200);
 const auto session=client.get_session();CHECK(session.authenticated&&session.user->id=="u1");
 client.sign_out();CHECK(!client.get_session().authenticated);
 CHECK(client.get_auth_headers().empty());
 bool rejected=false;
 try {gut::luth_http_request bad;bad.url="file:///etc/passwd";(void)curl(bad);}catch(const std::invalid_argument&){rejected=true;}
 CHECK(rejected);
 // Host-prefix attacks must fail before any DNS lookup or HTTP request.
 for(const std::string host: {"localhost.attacker.example", "127.0.0.1.evil.example", "[::1].evil.example"}) {
   rejected=false;
   try {gut::luth_http_request bad;bad.url="http://"+host+"/api/session";(void)curl(bad);}
   catch(const std::invalid_argument&){rejected=true;}
   CHECK(rejected);
 }
}
