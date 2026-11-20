#pragma once
// Optional libcurl-backed HTTPS transport. Not included by portable core headers.
// Compile/link this adapter explicitly with libcurl; all cookies stay in-process.
#include "gut/application_auth_client.hpp"
#include "gut/luth_client.hpp"
#include <curl/curl.h>
#include <algorithm>
#include <limits>
#include <map>
#include <string_view>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace gut {
class curl_http_adapter {
 struct curl_destroy {void operator()(CURL* ptr)const noexcept{if(ptr)curl_easy_cleanup(ptr);}};
 struct header_destroy {void operator()(curl_slist* ptr)const noexcept{if(ptr)curl_slist_free_all(ptr);}};
 std::unique_ptr<CURL,curl_destroy> handle_;
 std::mutex mutex_;
 const std::size_t max_bytes_;

 static void ensure_global() {
   static const bool ready=[](){return curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK;}();
   if(!ready)throw std::runtime_error("libcurl global initialization failed");
 }
 static std::size_t consume(char* data,std::size_t size,std::size_t count,void* user) noexcept {
   auto* state=static_cast<std::pair<std::string*,std::size_t>*>(user);
   if(size!=0 && count>std::numeric_limits<std::size_t>::max()/size)return 0;
   const auto bytes=size*count;
   if(bytes>state->second||state->first->size()>state->second-bytes)return 0;
   try{state->first->append(data,bytes);}catch(...){return 0;}
   return bytes;
 }
 static std::size_t consume_header(char* data,std::size_t size,std::size_t count,void* user) noexcept {
   if(size!=0 && count>std::numeric_limits<std::size_t>::max()/size)return 0;
   const auto bytes=size*count;
   auto* obj=static_cast<std::map<std::string,std::string>*>(user);
   try {
     std::string_view line(data,bytes);
     const auto colon=line.find(':');if(colon==std::string_view::npos)return bytes;
     std::string key(line.substr(0,colon));for(char& c:key)if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
     auto start=line.find_first_not_of(" \t",colon+1);
     if(start==std::string_view::npos)return bytes;
     auto end=line.find_last_not_of("\r\n \t");
     if(end!=std::string_view::npos && end>=start)(*obj)[std::move(key)]=std::string(line.substr(start,end-start+1));
   }catch(...){return 0;}
   return bytes;
 }
 template <typename T> static void set(CURL* handle,CURLoption option,T value) {
   if(curl_easy_setopt(handle,option,value)!=CURLE_OK)
     throw std::runtime_error("libcurl option rejected");
 }
public:
 explicit curl_http_adapter(std::size_t max_response_bytes=4*1024*1024):max_bytes_(max_response_bytes){
   ensure_global();handle_.reset(curl_easy_init());
   if(!handle_)throw std::runtime_error("libcurl handle allocation failed");
 }
 curl_http_adapter(const curl_http_adapter&)=delete;
 curl_http_adapter& operator=(const curl_http_adapter&)=delete;
 luth_http_response operator()(const luth_http_request& req) {
   if(req.method!="GET"&&req.method!="POST")throw std::invalid_argument("Unsupported HTTP method");
   if(req.url.find('\0')!=std::string::npos || req.url.find('\r')!=std::string::npos || req.url.find('\n')!=std::string::npos)
      throw std::invalid_argument("Unsafe HTTP URL");
   const auto parsed=net_detail::parse_absolute_url(req.url);
   if(parsed.scheme!="https"&&parsed.scheme!="http")throw std::invalid_argument("Unsafe HTTP scheme");
   if(parsed.scheme=="http"&&!luth_detail::loopback_authority(parsed.authority))
      throw std::invalid_argument("Plain HTTP is restricted to an exact loopback host");
   std::lock_guard<std::mutex> lock(mutex_);
   auto* easy=handle_.get();curl_easy_reset(easy);
   luth_http_response reply;
   std::pair<std::string*,std::size_t> body_state{&reply.body,max_bytes_};
   set(easy,CURLOPT_URL,req.url.c_str());
   set(easy,CURLOPT_FOLLOWLOCATION,0L);
   set(easy,CURLOPT_SSL_VERIFYPEER,1L);
   set(easy,CURLOPT_SSL_VERIFYHOST,2L);
   set(easy,CURLOPT_NOSIGNAL,1L);
   set(easy,CURLOPT_TIMEOUT_MS,static_cast<long>(std::max(1,req.timeout_ms)));
   set(easy,CURLOPT_CONNECTTIMEOUT_MS,static_cast<long>(std::max(1,req.timeout_ms)));
   set(easy,CURLOPT_WRITEFUNCTION,&consume);
   set(easy,CURLOPT_WRITEDATA,&body_state);
   set(easy,CURLOPT_HEADERFUNCTION,&consume_header);
   set(easy,CURLOPT_HEADERDATA,&reply.headers);
   // Empty COOKIEFILE enables in-memory cookies; never serialize them to disk.
   set(easy,CURLOPT_COOKIEFILE,"");
   if(req.method=="POST"){
     set(easy,CURLOPT_POST,1L);
     set(easy,CURLOPT_POSTFIELDS,req.body.c_str());
     set(easy,CURLOPT_POSTFIELDSIZE_LARGE,static_cast<curl_off_t>(req.body.size()));
   }
   std::unique_ptr<curl_slist,header_destroy> head;
   for(const auto& [key,value]:req.headers){
     if(key.find_first_of("\r\n")!=std::string::npos || value.find_first_of("\r\n")!=std::string::npos || key.find('\0')!=std::string::npos || value.find('\0')!=std::string::npos)
       throw std::invalid_argument("Unsafe HTTP header");
     auto* extended=curl_slist_append(head.get(),(key+": "+value).c_str());
     if(!extended)throw std::bad_alloc();
     head.release();head.reset(extended);
   }
   if(head)set(easy,CURLOPT_HTTPHEADER,head.get());
   const auto status=curl_easy_perform(easy);
   if(status!=CURLE_OK)throw std::runtime_error(std::string("libcurl request failed: ")+curl_easy_strerror(status));
   long response_status=0;
   if(curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&response_status)!=CURLE_OK)
     throw std::runtime_error("libcurl response status unavailable");
   reply.status=static_cast<int>(response_status);
   return reply;
 }
 application_auth_response fetch_application(const std::string& origin,const application_auth_request& req) {
   if(!valid_local_auth_path(req.path.substr(0,req.path.find('?'))))
     throw std::invalid_argument("Application fetch requires local API path");
   const auto parsed=net_detail::parse_absolute_url(origin);
   if(parsed.path!="/"||parsed.query_present||parsed.fragment_present||parsed.authority.empty())
     throw std::invalid_argument("Application fetch requires bare origin");
   if(!req.same_origin_credentials||!req.no_store)
     throw std::invalid_argument("Application fetch must use same-origin cookies and no-store");
   const luth_http_request call{req.method,parsed.origin()+req.path,req.headers,req.body,10000,true,true};
   const auto result=(*this)(call);
   return {result.status,result.body};
 }
};
} // namespace gut
