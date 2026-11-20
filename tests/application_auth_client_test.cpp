#include "gut/application_auth_client.hpp"
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

static std::string error_code(const std::function<void()>& fn){
 try{fn();}catch(const gut::luth_auth_error& e){return e.code();}
 return {};
}
int main(){
 std::vector<gut::application_auth_request> seen;
 std::vector<std::string> paths;
 bool logged_in=true;
 auto fetch=[&](const gut::application_auth_request& req)->gut::application_auth_response{
   seen.push_back(req);
   if(req.path=="/api/auth/oidc/logout"){
     assert(req.method=="POST"&&req.body=="{}");logged_in=false;return {200,"{}"};
   }
   if(req.path=="/api/auth/oidc/session"){
     assert(req.method=="GET"&&req.same_origin_credentials&&req.no_store);
     if(logged_in)return {200,R"({"authenticated":true,"user":{"id":"u1","name":"A","email":"a@b.com"},"session":{"id":"s1","userId":"u1","token":"SHOULD_NOT_LEAK","expiresAt":"2099"}})"};
     return {401,""};
   }
   return {404,"{}"};
 };
 gut::application_auth_client client(fetch,[&](const std::string& path){paths.push_back(path);});
 assert(client.get_auth_headers().empty());
 client.sign_in_with_google("/dashboard?x=hello world");
 assert(paths.size()==1&&paths[0]=="/api/auth/oidc/start?next=%2Fdashboard%3Fx%3Dhello%20world");
 client.sign_in_with_email("/other");assert(paths.size()==2);
 assert(error_code([&]{client.start_sign_in("//attacker.com");})=="INVALID_AUTH_RETURN_PATH");
 assert(error_code([&]{client.start_sign_in("/bad\\path");})=="INVALID_AUTH_RETURN_PATH");
 assert(error_code([&]{gut::application_auth_client invalid(fetch,{},"//evil");})=="INVALID_APPLICATION_AUTH_PATH");
 assert(error_code([&]{gut::application_auth_client invalid(fetch,{},"/api/%2e%2e");})=="INVALID_APPLICATION_AUTH_PATH");
 const auto session=client.get_session();assert(session.authenticated && session.user->id=="u1"&&session.session->id=="s1");
 assert(!session.session->token);
 client.sign_out();assert(!client.get_session().authenticated);
 for(const auto& request:seen)assert(request.headers.find("Authorization")==request.headers.end());
 const auto failing=[](const gut::application_auth_request&)->gut::application_auth_response{throw std::runtime_error("network");};
 gut::application_auth_client offline(failing);
 assert(error_code([&]{(void)offline.get_session();})=="AUTH_SESSION_UNAVAILABLE");
 assert(error_code([&]{offline.sign_out();})=="AUTH_LOGOUT_UNAVAILABLE");
 gut::application_auth_client rejected([](const gut::application_auth_request&)->gut::application_auth_response{return {503,""};});
 assert(error_code([&]{(void)rejected.get_session();})=="AUTH_SESSION_UNAVAILABLE");
 gut::application_auth_client bad_payload([](const gut::application_auth_request&)->gut::application_auth_response{return {200,R"({"authenticated":true,"user":{},"session":{"id":"s1"}})"};});
 assert(error_code([&]{(void)bad_payload.get_session();})=="AUTH_SESSION_INVALID");
 gut::application_auth_client no_nav(fetch);
 assert(error_code([&]{no_nav.start_sign_in("/");})=="NAVIGATION_UNAVAILABLE");
}