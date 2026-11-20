#pragma once
// Opt-in native S3 REST adapter with AWS Signature Version 4 (SigV4).
// Requires OpenSSL libcrypto at link time. No AWS SDK, Node.js or browser runtime.
// Credentials and transport are supplied by the caller and never logged.
#include "gut/luth_client.hpp" // Reusable native HTTP request/response contracts.
#include "gut/percent_encoding.hpp"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
class s3_error : public std::runtime_error {
  int status_;
public:
  explicit s3_error(const char* message,int status=0):std::runtime_error(message),status_(status){}
  int status()const noexcept{return status_;}
};
struct s3_credentials {
  std::string access_key_id;
  std::string secret_access_key;
  std::string session_token;
};
struct s3_options {
  std::string region="us-east-1";
  std::string endpoint_origin="https://s3.amazonaws.com";
  bool allow_loopback_test_origin=false;
  int timeout_ms=30000;
  std::size_t max_response_bytes=64u*1024u*1024u;
};
namespace s3_detail {
inline void assert_header_value(std::string_view v) {
  if(v.size()>16384 || v.find('\0')!=std::string_view::npos || v.find('\r')!=std::string_view::npos || v.find('\n')!=std::string_view::npos)
    throw std::invalid_argument("Unsafe S3 credential/header value");
}
inline std::string hex(std::string_view bytes) {
  constexpr char dig[]="0123456789abcdef";
  std::string out;out.reserve(bytes.size()*2);
  for(unsigned char c:bytes){out.push_back(dig[c>>4]);out.push_back(dig[c&15]);}return out;
}
inline std::string hmac(std::string_view key,std::string_view bytes){
  if(key.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))throw std::invalid_argument("Excessive S3 signing key size");
  unsigned char out[EVP_MAX_MD_SIZE];unsigned len=0;
  if(!HMAC(EVP_sha256(),key.data(),static_cast<int>(key.size()),
           reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),out,&len) || len!=32)
    throw std::runtime_error("S3 HMAC-SHA256 failed");
  return {reinterpret_cast<const char*>(out),len};
}
inline std::string sha256(std::string_view bytes){
  std::array<unsigned char,EVP_MAX_MD_SIZE> out{};unsigned len=0;
  if(EVP_Digest(bytes.data(),bytes.size(),out.data(),&len,EVP_sha256(),nullptr)!=1||len!=32)
    throw std::runtime_error("S3 SHA-256 failed");
  return {reinterpret_cast<const char*>(out.data()),len};
}
inline std::string md5_base64(std::string_view bytes){
  std::array<unsigned char,EVP_MAX_MD_SIZE> out{};unsigned len=0;
  if(EVP_Digest(bytes.data(),bytes.size(),out.data(),&len,EVP_md5(),nullptr)!=1||len!=16)
    throw std::runtime_error("S3 Content-MD5 generation failed");
  std::array<unsigned char,32> base64{};
  const int n=EVP_EncodeBlock(base64.data(),out.data(),static_cast<int>(len));
  if(n!=24)throw std::runtime_error("S3 Content-MD5 encoding failed");
  return {reinterpret_cast<const char*>(base64.data()),static_cast<std::size_t>(n)};
}
inline std::string current_amz_date(){
  std::time_t now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm utc{};
#if defined(_WIN32)
  if(gmtime_s(&utc,&now)!=0)throw std::runtime_error("Unable to derive S3 UTC timestamp");
#else
  if(gmtime_r(&now,&utc)==nullptr)throw std::runtime_error("Unable to derive S3 UTC timestamp");
#endif
  char out[17]{};
  if(std::strftime(out,sizeof(out),"%Y%m%dT%H%M%SZ",&utc)!=16)throw std::runtime_error("Unable to format S3 timestamp");
  return out;
}
inline void valid_amz_date(std::string_view date){
  if(date.size()!=16 || date[8]!='T' || date[15]!='Z')throw std::invalid_argument("Invalid AWS SigV4 timestamp");
  for(std::size_t i=0;i<date.size();++i)if(i!=8&&i!=15&&(date[i]<'0'||date[i]>'9'))
    throw std::invalid_argument("Invalid AWS SigV4 timestamp");
}
inline void valid_region(std::string_view region){
  if(region.empty()||region.size()>63)throw std::invalid_argument("Invalid AWS region");
  for(char ch:region)if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='-'))
    throw std::invalid_argument("Invalid AWS region");
}
inline void valid_bucket(std::string_view bucket){
  if(bucket.size()<3||bucket.size()>63||bucket.front()=='.'||bucket.back()=='.'||bucket.front()=='-'||bucket.back()=='-')
    throw std::invalid_argument("Invalid S3 bucket name");
  for(char ch:bucket)if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='.'))
    throw std::invalid_argument("Invalid S3 bucket name");
  if(bucket.find("..")!=std::string_view::npos)throw std::invalid_argument("Invalid S3 bucket name");
}
inline std::string encoded_key(std::string_view key){
  if(key.empty()||key.size()>4096||key.find('\0')!=std::string_view::npos)
    throw std::invalid_argument("Invalid S3 object key");
  // S3 object keys are UTF-8; do not silently reinterpret invalid bytes.
  (void)utf8_scalars(key);
  // No ambiguous dot-segments are allowed in this portable URL subset.
  std::string path;std::size_t pos=0;bool first=true;
  while(pos<=key.size()){
    auto end=key.find('/',pos);if(end==std::string_view::npos)end=key.size();
    const auto segment=key.substr(pos,end-pos);
    if(segment=="."||segment=="..")throw std::invalid_argument("S3 URL dot segments are unsafe");
    if(!first)path+='/';
    first=false;
    path+=percent_encode_component(segment);
    if(end==key.size())break;
    pos=end+1;
  }
  return path;
}
inline std::string encoded_query(std::vector<std::pair<std::string,std::string>> items){
  for(auto& pair:items){pair.first=percent_encode_component(pair.first);pair.second=percent_encode_component(pair.second);}
  std::sort(items.begin(),items.end());
  std::string out;
  for(const auto& [key,value]:items){if(!out.empty())out+='&';out+=key+'='+value;}
  return out;
}
inline std::string xml_escape(std::string_view input){
  (void)utf8_scalars(input);
  std::string out;
  for(char ch:input){
    switch(ch){case '&':out+="&amp;";break;case '<':out+="&lt;";break;
      case '>':out+="&gt;";break;case '\'':out+="&apos;";break;case '"':out+="&quot;";break;
      default:
        if(static_cast<unsigned char>(ch)<32&&ch!='\n'&&ch!='\r'&&ch!='\t')
          throw std::invalid_argument("Invalid S3 XML key");
        out+=ch;
        break;
    }
  }return out;
}
inline std::string canonical_headers(const std::map<std::string,std::string>& headers){
  std::string out;
  for(const auto& [name,value]:headers){out+=name+':'+value+'\n';}
  return out;
}
inline std::string signed_headers(const std::map<std::string,std::string>& headers){
  std::string out;for(const auto& [key,_]:headers){if(!out.empty())out+=';';out+=key;}return out;
}
inline std::string signature(const s3_credentials& c,std::string_view region,std::string_view timestamp,
                             std::string_view canonical_request){
  valid_amz_date(timestamp);
  const std::string date(timestamp.substr(0,8));
  const std::string scope=date+"/"+std::string(region)+"/s3/aws4_request";
  const std::string to_sign="AWS4-HMAC-SHA256\n"+std::string(timestamp)+"\n"+scope+"\n"+hex(sha256(canonical_request));
  const auto date_key=hmac("AWS4"+c.secret_access_key,date);
  const auto region_key=hmac(date_key,region);
  const auto service_key=hmac(region_key,"s3");
  const auto final_key=hmac(service_key,"aws4_request");
  return hex(hmac(final_key,to_sign));
}
}
class s3_client {
  luth_http_transport transport_;
  std::function<s3_credentials()> credentials_;
  std::function<std::string()> clock_;
  std::string origin_,host_,region_;
  int timeout_ms_;
  std::size_t max_response_bytes_;
  luth_http_response request(const char* method,const std::string& path,
                             std::vector<std::pair<std::string,std::string>> query={},
                             std::string body={},std::map<std::string,std::string> extra={}) const {
    const auto c=credentials_();
    if(c.access_key_id.empty()||c.secret_access_key.empty())throw s3_error("Missing AWS S3 credentials");
    s3_detail::assert_header_value(c.access_key_id);s3_detail::assert_header_value(c.secret_access_key);
    s3_detail::assert_header_value(c.session_token);
    const auto date=clock_();s3_detail::valid_amz_date(date);
    const std::string payload_hash=s3_detail::hex(s3_detail::sha256(body));
    std::map<std::string,std::string> headers{
      {"host",host_}, {"x-amz-content-sha256",payload_hash}, {"x-amz-date",date}
    };
    if(!c.session_token.empty())headers.emplace("x-amz-security-token",c.session_token);
    for(const auto& [key,value]:extra){
      if(key.empty()||key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos)
        throw std::invalid_argument("Invalid S3 header name");
      s3_detail::assert_header_value(value);
      if(!headers.emplace(key,value).second)throw std::invalid_argument("Duplicate S3 signed header");
    }
    const auto query_string=s3_detail::encoded_query(std::move(query));
    const std::string canonical=std::string(method)+"\n"+path+"\n"+query_string+"\n"+
      s3_detail::canonical_headers(headers)+"\n"+s3_detail::signed_headers(headers)+"\n"+payload_hash;
    const auto signature=s3_detail::signature(c,region_,date,canonical);
    const auto scope=date.substr(0,8)+"/"+region_+"/s3/aws4_request";
    headers.emplace("authorization","AWS4-HMAC-SHA256 Credential="+c.access_key_id+"/"+scope+
                    ",SignedHeaders="+s3_detail::signed_headers(headers)+",Signature="+signature);
    luth_http_request req;req.method=method;req.url=origin_+path+(query_string.empty()?"":"?"+query_string);
    req.body=std::move(body);req.headers=std::move(headers);req.timeout_ms=timeout_ms_;
    req.credentials_include=false;req.no_store=true;
    const auto response=transport_(req);
    if(response.body.size()>max_response_bytes_)throw s3_error("S3 response exceeds configured size",response.status);
    if(response.status<200||response.status>=300)throw s3_error("AWS S3 request failed",response.status);
    return response;
  }
  static std::string bucket_path(std::string_view bucket){
    s3_detail::valid_bucket(bucket);return "/"+std::string(bucket);
  }
  static std::string object_path(std::string_view bucket,std::string_view key){
    return bucket_path(bucket)+"/"+s3_detail::encoded_key(key);
  }
public:
  s3_client(luth_http_transport transport,std::function<s3_credentials()> credentials,
            s3_options options={},std::function<std::string()> clock=s3_detail::current_amz_date)
     :transport_(std::move(transport)),credentials_(std::move(credentials)),clock_(std::move(clock)),
      origin_(std::move(options.endpoint_origin)),region_(std::move(options.region)),
      timeout_ms_(options.timeout_ms),max_response_bytes_(options.max_response_bytes){
    if(!transport_||!credentials_||!clock_||timeout_ms_<=0||timeout_ms_>600000||
       max_response_bytes_==0||max_response_bytes_>512u*1024u*1024u)
      throw std::invalid_argument("Invalid S3 client configuration");
    s3_detail::valid_region(region_);
    const auto p=net_detail::parse_absolute_url(origin_);
    if((p.scheme!="https"&& !(options.allow_loopback_test_origin&&p.scheme=="http"&&
        luth_detail::loopback_authority(p.authority)))||p.query_present||p.fragment_present||
       p.path!="/"||p.origin()!=origin_)
      throw std::invalid_argument("S3 endpoint must be a bare trusted HTTPS origin or an explicit loopback test origin");
    host_=p.authority;
  }
  std::string get_object(std::string_view bucket,std::string_view key) const{
    return request("GET",object_path(bucket,key)).body;
  }
  std::map<std::string,std::string> head_object(std::string_view bucket,std::string_view key) const{
    return request("HEAD",object_path(bucket,key)).headers;
  }
  std::map<std::string,std::string> put_object(std::string_view bucket,std::string_view key,
                                               std::string bytes,std::string content_type="application/octet-stream") const {
    s3_detail::assert_header_value(content_type);if(content_type.empty())throw std::invalid_argument("Invalid S3 content type");
    return request("PUT",object_path(bucket,key),{},std::move(bytes),{{"content-type",std::move(content_type)}}).headers;
  }
  std::string list_objects_v2(std::string_view bucket,std::string prefix={},
                               int max_keys=1000,std::string continuation_token={}) const {
    if(max_keys<1||max_keys>1000||prefix.size()>4096||continuation_token.size()>4096)
      throw std::invalid_argument("Invalid S3 list parameters");
    std::vector<std::pair<std::string,std::string>> query{{"list-type","2"},{"max-keys",std::to_string(max_keys)}};
    if(!prefix.empty())query.emplace_back("prefix",std::move(prefix));
    if(!continuation_token.empty())query.emplace_back("continuation-token",std::move(continuation_token));
    return request("GET",bucket_path(bucket),std::move(query)).body;
  }
  std::string copy_object(std::string_view dst_bucket,std::string_view dst_key,
                           std::string_view source_bucket,std::string_view source_key) const {
    return request("PUT",object_path(dst_bucket,dst_key),{}, {},
     {{"x-amz-copy-source",object_path(source_bucket,source_key)}}).body;
  }
  void delete_object(std::string_view bucket,std::string_view key) const {
    (void)request("DELETE",object_path(bucket,key));
  }
  std::string delete_objects(std::string_view bucket,const std::vector<std::string>& keys,
                             bool quiet=false) const {
    if(keys.empty()||keys.size()>1000)throw std::invalid_argument("Invalid S3 DeleteObjects list");
    std::string xml="<Delete xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
    for(const auto& key:keys){(void)s3_detail::encoded_key(key);xml+="<Object><Key>"+s3_detail::xml_escape(key)+"</Key></Object>";}
    if(quiet)xml+="<Quiet>true</Quiet>";
    xml+="</Delete>";
    const auto md5=s3_detail::md5_base64(xml);
    return request("POST",bucket_path(bucket),{{"delete",""}},std::move(xml),
       {{"content-type","application/xml"},{"content-md5",md5}}).body;
  }
};
} // namespace gut
