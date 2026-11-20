#pragma once
// Native C++17 Luth HTTP protocol client. Pinned to GutTS auth/luthAuthClient.ts.
// Caller owns the actual HTTP transport and a secure token store. Never log secrets.
#include "gut/json.hpp"
#include "gut/luth_types.hpp"
#include "gut/net.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace gut {
struct luth_http_request {
 std::string method="GET";
 std::string url;
 std::map<std::string,std::string> headers;
 std::string body;
 int timeout_ms=10000;
 bool no_store=true;
 bool credentials_include=true;
};
struct luth_http_response {
 int status=0;
 std::string body;
 std::map<std::string,std::string> headers;
 bool ok()const noexcept{return status>=200&&status<300;}
};
using luth_http_transport=std::function<luth_http_response(const luth_http_request&)>;
inline constexpr const char* default_luth_url="https://luth.blueprint-ninjastudios.workers.dev";
namespace luth_detail {
inline deployment_environment environment_for_origin(const std::string& origin) {
 const auto parsed=net_detail::parse_absolute_url(origin);
 std::string host=parsed.authority;for(char& c:host)if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
 if(host=="localhost" || host.rfind("localhost:",0)==0 || host=="127.0.0.1" || host.rfind("127.0.0.1:",0)==0 || host=="[::1]" || host.rfind("[::1]:",0)==0)return deployment_environment::development;
 if(host.find("staging")!=std::string::npos)return deployment_environment::staging;
 if(host.find("preview")!=std::string::npos || host.find("-git-")!=std::string::npos || (host.size()>=12 && host.substr(host.size()-12)==".workers.dev"))return deployment_environment::preview;
 return deployment_environment::production;
}
inline std::string normalized_base(std::string url) {
 while(!url.empty()&&url.back()=='/')url.pop_back();
 const auto parsed=net_detail::parse_absolute_url(url);
 const bool loopback = parsed.authority == "localhost" || parsed.authority.rfind("localhost:",0)==0 ||
   parsed.authority == "127.0.0.1" || parsed.authority.rfind("127.0.0.1:",0)==0 ||
   parsed.authority == "[::1]" || parsed.authority.rfind("[::1]:",0)==0;
 if((parsed.scheme!="https" && !(parsed.scheme=="http"&&loopback)) ||
    parsed.path!="/"||parsed.query_present||parsed.fragment_present)
   throw luth_auth_error("INVALID_LUTH_URL","Luth endpoint must be HTTPS or loopback development HTTP.");
 return url;
}
inline std::string normalized_origin(const std::string& raw) {
 const auto p=net_detail::parse_absolute_url(raw);
 if((p.scheme!="https"&&p.scheme!="http")||p.authority.empty()||p.query_present||p.fragment_present)
   throw luth_auth_error("APPLICATION_ORIGIN_UNAVAILABLE","Application origin is unavailable.");
 if(p.scheme=="http"&&p.authority.rfind("localhost",0)!=0 && p.authority.rfind("127.0.0.1",0)!=0 && p.authority.rfind("[::1]",0)!=0)
   throw luth_auth_error("APPLICATION_ORIGIN_UNAVAILABLE","Plain HTTP requires loopback origin.");
 return p.origin();
}
inline std::optional<std::string> str(const json_value::object& obj,const std::string& key) {
 auto it=obj.find(key);if(it==obj.end())return std::nullopt;
 if(const auto* s=std::get_if<std::string>(&it->second.data))return *s;
 return std::nullopt;
}
inline std::optional<bool> boolean(const json_value::object& obj,const std::string& key) {
 auto it=obj.find(key);if(it==obj.end())return std::nullopt;
 if(const auto* b=std::get_if<bool>(&it->second.data))return *b;
 return std::nullopt;
}
inline json_value::object parse_object(const std::string& raw) {
 const auto value=try_parse_json(raw);
 if(!value || !std::holds_alternative<json_value::object>(value->data))
   throw luth_auth_error("INVALID_LUTH_RESPONSE","Luth returned an invalid JSON object.");
 return value->as<json_value::object>();
}
inline std::string response_token(const luth_http_response& response,const json_value::object& payload) {
 for(const auto& [key,value]:response.headers){
   std::string normalized=key;
   for(char& c:normalized)if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
   if(normalized=="set-auth-token"&& !value.empty())return value;
 }
 if(const auto token=str(payload,"token");token && !token->empty())return *token;
 const auto it=payload.find("session");
 if(it!=payload.end())if(const auto* nested=std::get_if<json_value::object>(&it->second.data))
   return str(*nested,"token").value_or("");
 return {};
}
inline luth_session_result session_from_json(const std::string& raw) {
 const auto obj=parse_object(raw);
 const auto auth=boolean(obj,"authenticated");
 if(!auth)throw luth_auth_error("INVALID_LUTH_RESPONSE","Luth session missing authenticated flag.");
 luth_session_result result;result.authenticated=*auth;
 if(const auto code=str(obj,"error"))result.error=*code;
 if(!*auth)return result;
 const auto u=obj.find("user"),s=obj.find("session");
 if(u==obj.end()||s==obj.end()||!std::holds_alternative<json_value::object>(u->second.data)||!std::holds_alternative<json_value::object>(s->second.data))
   throw luth_auth_error("INVALID_LUTH_RESPONSE","Authenticated session is incomplete.");
 const auto& user=u->second.as<json_value::object>();const auto& sess=s->second.as<json_value::object>();
 const auto uid=str(user,"id"),email=str(user,"email"),sid=str(sess,"id");
 if(!uid||uid->empty()||!email||email->empty()||!sid||sid->empty())
   throw luth_auth_error("INVALID_LUTH_RESPONSE","Authenticated session is incomplete.");
 result.user=luth_user{*uid,str(user,"name").value_or(""),*email,str(user,"image"),boolean(user,"emailVerified")};
 result.session=luth_session{*sid,str(sess,"userId").value_or(*uid),str(sess,"expiresAt").value_or(""),str(sess,"token")};
 return result;
}
inline json_value::object make_body(const std::map<std::string,std::string>& values){
 json_value::object result;for(const auto& [k,v]:values)result.emplace(k,v);return result;
}
inline std::string error_message(const json_value::object& o,const std::string& fallback){return str(o,"message").value_or(fallback);}
inline std::string error_code(const json_value::object& o,const std::string& fallback){return str(o,"code").value_or(fallback);}
} // namespace luth_detail

class luth_client {
 const std::string ecosystem_id_,application_id_,origin_;
 const deployment_environment environment_;
 std::vector<std::string> urls_;
 std::size_t active_index_=0,next_index_=0;
 const std::function<std::vector<std::string>()> resolver_;
 const bool auto_register_;
 const int max_retries_,retry_delay_ms_,timeout_ms_;
 luth_http_transport transport_;
 std::shared_ptr<luth_auth_token_store> tokens_;
 std::function<void(int)> sleep_ms_;
 std::mutex mutex_;
 std::mutex register_mutex_;
 std::optional<register_application_result> registered_;

 luth_http_response request(std::string path,std::string method="GET",std::string body="") {
   if(path.empty()||path.front()!='/'||(path.size()>1&&path[1]=='/')||path.find('\\')!=std::string::npos||path.find('#')!=std::string::npos)
      throw luth_auth_error("INVALID_LUTH_PATH","Untrusted request path.");
   std::vector<std::string> urls;
   std::size_t start=0;
   {
     std::lock_guard<std::mutex> lock(mutex_);
     urls=urls_;start=urls.size()>1?next_index_%urls.size():0;
   }
   if(resolver_&&urls.size()<=1){
     try {
       const auto discovered=resolver_();
       std::lock_guard<std::mutex> lock(mutex_);
       std::vector<std::string> refreshed;
       for(const auto& original:discovered){
         if(original.empty())continue;
         const auto candidate=luth_detail::normalized_base(original);
         if(std::find(refreshed.begin(),refreshed.end(),candidate)==refreshed.end())refreshed.push_back(candidate);
       }
       for(const auto& existing:urls_)if(std::find(refreshed.begin(),refreshed.end(),existing)==refreshed.end())refreshed.push_back(existing);
       urls_=std::move(refreshed);
       urls=urls_;start=urls.size()>1?next_index_%urls.size():0;
     } catch(...) {} // Resolver is optional: existing verified endpoints remain available.
   }
   if(urls.empty())throw luth_auth_error("LUTH_DEPLOYMENT_UNAVAILABLE","No Luth endpoints configured.");
   std::optional<luth_auth_error> last_error;
   for(int cycle=0;cycle<=max_retries_;++cycle){
     for(std::size_t offset=0;offset<urls.size();++offset){
       const auto index=(start+offset)%urls.size();
       luth_http_request req;req.url=urls[index]+path;req.method=method;req.body=body;req.timeout_ms=timeout_ms_;
       if(!body.empty())req.headers["Content-Type"]="application/json";
       const auto token=tokens_->get();if(!token.empty())req.headers["Authorization"]="Bearer "+token;
       try {
         const auto response=transport_(req);
         if(response.status==502||response.status==503||response.status==504){
           last_error=luth_auth_error("LUTH_DEPLOYMENT_UNAVAILABLE","Luth deployment unavailable",response.status);
           continue;
         }
         {std::lock_guard<std::mutex> lock(mutex_);
           active_index_=index;
           next_index_=offset==0?(index+1)%urls.size():index;
         }
         return response;
       } catch(const luth_auth_error& e) {last_error=e;}
       catch(const std::exception&) {last_error=luth_auth_error("LUTH_DEPLOYMENT_UNAVAILABLE","Luth request transport failed.");}
     }
     if(cycle<max_retries_)sleep_ms_(static_cast<int>(std::min<std::int64_t>(static_cast<std::int64_t>(retry_delay_ms_)*(std::int64_t{1}<<std::min(cycle,12)),60000)));
   }
   throw last_error.value_or(luth_auth_error("LUTH_DEPLOYMENT_UNAVAILABLE","All Luth endpoints exhausted."));
 }
 json_value::object post_auth(const std::string& path,const json_value::object& data,const std::string& fallback){
   auto response=request(path,"POST",stringify_json(data));
   const auto payload=luth_detail::parse_object(response.body);
   const auto token=luth_detail::response_token(response,payload);
   if(!token.empty())tokens_->set(token);
   if(!response.ok())throw luth_auth_error(luth_detail::error_code(payload,"AUTHENTICATION_FAILED"),luth_detail::error_message(payload,fallback),response.status);
   return payload;
 }
public:
 explicit luth_client(luth_auth_client_config config,luth_http_transport transport,
  std::function<void(int)> sleeper=[](int ms){std::this_thread::sleep_for(std::chrono::milliseconds(ms));})
 :ecosystem_id_(std::move(config.ecosystem_id)),application_id_(std::move(config.application_id)),
  origin_(luth_detail::normalized_origin(config.origin.value_or(""))),
  environment_(config.environment.value_or(luth_detail::environment_for_origin(origin_))),
  resolver_(std::move(config.luth_url_resolver)),
  auto_register_(config.auto_register.value_or(true)),max_retries_(config.max_retries.value_or(3)),
  retry_delay_ms_(config.retry_delay_ms.value_or(100)),timeout_ms_(config.timeout_ms.value_or(10000)),
  transport_(std::move(transport)),tokens_(std::move(config.token_store)),sleep_ms_(std::move(sleeper)) {
    if(!valid_identity_id("application",application_id_)||ecosystem_id_.empty()||
      !transport_||!tokens_||max_retries_<0||max_retries_>20||retry_delay_ms_<0||timeout_ms_<0)
      throw luth_auth_error("INVALID_LUTH_CLIENT_CONFIG","Luth client needs explicit origin, transport and token store.");
    const auto preferred=config.luth_urls.empty()?std::vector<std::string>{config.luth_url.value_or(default_luth_url)}:config.luth_urls;
    for(const auto& url:preferred){const auto normalized=luth_detail::normalized_base(url);
      if(std::find(urls_.begin(),urls_.end(),normalized)==urls_.end())urls_.push_back(normalized);}
 }
 const std::string& origin()const noexcept{return origin_;}
 std::string luth_url(){std::lock_guard<std::mutex> lock(mutex_);return urls_.at(active_index_);}
 std::map<std::string,std::string> get_auth_headers()const {
   const auto token=tokens_->get();return token.empty()?std::map<std::string,std::string>{}:std::map<std::string,std::string>{{"Authorization","Bearer "+token}};
 }
 void clear_auth_token(){tokens_->clear();}
 register_application_result register_application(){
   luth_auth_error last("REGISTRATION_FAILED","Application registration failed.");
   for(int attempt=0;attempt<=max_retries_;++attempt){
     try {
       auto response=request("/api/registry/register","POST",stringify_json(luth_detail::make_body({{"ecosystemId",ecosystem_id_},{"applicationId",application_id_},{"origin",origin_},{"environment",environment_==deployment_environment::development?"development":environment_==deployment_environment::preview?"preview":environment_==deployment_environment::staging?"staging":"production"}})));
       const auto body=luth_detail::parse_object(response.body);
       if(response.ok()){
         auto registered=luth_detail::boolean(body,"registered"),enabled=luth_detail::boolean(body,"authenticationEnabled");
         if(!registered||!enabled)throw luth_auth_error("REGISTRATION_FAILED","Malformed Luth registration response.");
         return {*registered,luth_detail::str(body,"ecosystemId").value_or(ecosystem_id_),
          luth_detail::str(body,"applicationId").value_or(application_id_),
          luth_detail::str(body,"origin").value_or(origin_),environment_,*enabled};
       }
       const auto code=luth_detail::error_code(body,response.status==404?"APPLICATION_NOT_REGISTERED":"REGISTRATION_FAILED");
       last=luth_auth_error(code,luth_detail::error_message(body,"Application registration failed"),response.status);
       if(response.status<500)throw last;
     } catch(const luth_auth_error& e) {
       if(e.status()&&*e.status()<500)throw;
       last=e;
     }
     if(attempt<max_retries_)sleep_ms_(static_cast<int>(std::min<std::int64_t>(static_cast<std::int64_t>(retry_delay_ms_)*(std::int64_t{1}<<std::min(attempt,12)),60000)));
   }
   throw luth_auth_error("REGISTRATION_FAILED",last.what());
 }
 std::optional<register_application_result> ensure_registered() {
   if(!auto_register_)return std::nullopt;
   std::lock_guard<std::mutex> lock(register_mutex_);
   if(!registered_)registered_=register_application();
   return registered_;
 }
 luth_session_result get_session(){
   (void)ensure_registered();
   const auto response=request("/api/session");
   for(const auto& [key,value]:response.headers){
     std::string n=key;for(char& c:n)if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
     if(n=="set-auth-token"&&!value.empty())tokens_->set(value);
   }
   if(response.status==401){clear_auth_token();return {false,std::nullopt,std::nullopt,std::string("SESSION_INVALID")};}
   if(!response.ok())throw luth_auth_error("AUTHENTICATION_FAILED","Session fetch failed",response.status);
   auto result=luth_detail::session_from_json(response.body);
   if(result.session&&result.session->token)tokens_->set(*result.session->token);
   return result;
 }
 luth_session_result refresh_session(){return get_session();}
 json_value::object sign_in_with_email(const email_sign_in_credentials& credentials){
   (void)ensure_registered();return post_auth("/api/auth/sign-in/email",luth_detail::make_body({{"email",credentials.email},{"password",credentials.password}}),"Sign in failed");
 }
 json_value::object sign_up_with_email(const email_sign_up_data& data){
   (void)ensure_registered();return post_auth("/api/auth/sign-up/email",luth_detail::make_body({{"email",data.email},{"password",data.password},{"name",data.name}}),"Sign up failed");
 }
 json_value::object sign_in_with_google(const google_sign_in_options& options={}){
   (void)ensure_registered();return post_auth("/api/auth/sign-in/social",luth_detail::make_body({{"provider","google"},{"callbackURL",options.callback_url.value_or(origin_)}}),"Google sign-in failed");
 }
 void sign_out(){
   const auto response=request("/api/auth/sign-out","POST","{}");
   const auto payload=luth_detail::parse_object(response.body);
   if(!response.ok())throw luth_auth_error(luth_detail::error_code(payload,"AUTHENTICATION_FAILED"),luth_detail::error_message(payload,"Sign out failed"),response.status);
   clear_auth_token();
   if(get_session().authenticated)
     throw luth_auth_error("SESSION_INVALIDATION_FAILED","Luth session is still active after sign-out.");
 }
};
} // namespace gut
