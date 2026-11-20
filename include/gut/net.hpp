#pragma once
// Portable URI component, query and trusted-origin operations.
#include "gut/portable.hpp"
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gut {
namespace net_detail {
inline int hex_digit(char c) noexcept {
  if(c>='0'&&c<='9')return c-'0';
  if(c>='a'&&c<='f')return c-'a'+10;
  if(c>='A'&&c<='F')return c-'A'+10;
  return -1;
}
inline bool ascii_alnum(unsigned char c) noexcept {
  return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9');
}
inline bool unreserved(unsigned char c) noexcept {
  return ascii_alnum(c)||c=='-'||c=='.'||c=='_'||c=='~';
}
inline bool js_component_safe(unsigned char c) noexcept {
  return unreserved(c)||c=='!'||c=='\''||c=='('||c==')'||c=='*';
}
inline std::string encode(std::string_view s,bool js_style,bool query_form=false) {
  constexpr char digits[]="0123456789ABCDEF";
  std::string out;out.reserve(s.size());
  for(unsigned char c:s){
    if(query_form && c==' ') out+='+';
    else if(unreserved(c)||(!query_form&&js_style&&js_component_safe(c))) out+=static_cast<char>(c);
    else {out+='%';out+=digits[c>>4];out+=digits[c&15];}
  }
  return out;
}
inline std::string decode(std::string_view s,bool plus_as_space=false) {
  std::string out;out.reserve(s.size());
  for(std::size_t i=0;i<s.size();++i) {
    if(plus_as_space && s[i]=='+'){out+=' ';continue;}
    if(s[i]!='%'){out+=s[i];continue;}
    if(i+2>=s.size())throw std::invalid_argument("malformed percent escape");
    const auto a=hex_digit(s[i+1]),b=hex_digit(s[i+2]);
    if(a<0||b<0)throw std::invalid_argument("malformed percent escape");
    out+=static_cast<char>((a<<4)|b);i+=2;
  }
  (void)utf8_scalars(out);
  return out;
}
inline void validate_segment(std::string segment,bool allow_slash) {
  for(unsigned depth=0;depth<8;++depth){
    segment=decode(segment);
    auto start=std::size_t{0};
    while(true){
      const auto end=segment.find('/',start);
      const auto sub=segment.substr(start,end==std::string::npos?std::string::npos:end-start);
      if(sub=="."||sub=="..")throw std::invalid_argument("unsafe relative segment");
      if(end==std::string::npos)break;
      start=end+1;
    }
    for(unsigned char c:segment){
      if(c=='\\'||c=='?'||c=='#'||c<32||c==127||(!allow_slash&&c=='/'))
        throw std::invalid_argument("unsafe relative segment");
    }
    bool encoded=false;
    for(std::size_t i=0;i+2<segment.size();++i)
      if(segment[i]=='%'&&hex_digit(segment[i+1])>=0&&hex_digit(segment[i+2])>=0)
        {encoded=true;break;}
    if(!encoded)return;
  }
  throw std::invalid_argument("path too deeply encoded");
}
struct parsed_url {
  std::string scheme,authority,path,query,fragment;
  bool query_present=false,fragment_present=false;
  std::string origin()const {return scheme+"://"+authority;}
  std::string serialized()const {
    auto output=origin()+path;
    if(query_present)output+='?'+query;
    if(fragment_present)output+='#'+fragment;
    return output;
  }
};
inline parsed_url parse_absolute_url(std::string_view input) {
  const auto mark=input.find("://");
  if(mark==std::string_view::npos||mark==0)throw std::invalid_argument("absolute URL required");
  const auto scheme=input.substr(0,mark);
  for(std::size_t i=0;i<scheme.size();++i) {
    const auto c=static_cast<unsigned char>(scheme[i]);
    if(!(c>='a'&&c<='z')&&!(c>='A'&&c<='Z')) {
      if(i==0||(!(c>='0'&&c<='9')&&(c!='+'&&c!='-'&&c!='.')))
        throw std::invalid_argument("invalid URL scheme");
    }
  }
  const auto begin=mark+3;
  const auto end=input.find_first_of("/?#",begin);
  const auto authority=input.substr(begin,end==std::string_view::npos?input.size()-begin:end-begin);
  if(authority.empty()||authority.find('@')!=std::string_view::npos)
    throw std::invalid_argument("invalid URL authority");
  for(unsigned char c:authority)if(c<=32||c==127||c=='\\')throw std::invalid_argument("invalid URL host");
  parsed_url out;
  out.scheme=std::string(scheme);
  for(char& c:out.scheme)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));
  out.authority=std::string(authority);
  std::string_view remainder=end==std::string_view::npos?std::string_view{}:input.substr(end);
  if(!remainder.empty()&&remainder.front()=='/'){
    const auto path_end=remainder.find_first_of("?#");
    out.path=std::string(remainder.substr(0,path_end));
    remainder=path_end==std::string_view::npos?std::string_view{}:remainder.substr(path_end);
  } else out.path="/";
  if(!remainder.empty()&&remainder.front()=='?'){
    out.query_present=true;const auto query_end=remainder.find('#');
    out.query=std::string(remainder.substr(1,query_end==std::string_view::npos?std::string_view::npos:query_end-1));
    remainder=query_end==std::string_view::npos?std::string_view{}:remainder.substr(query_end);
  }
  if(!remainder.empty()&&remainder.front()=='#'){
    out.fragment_present=true;out.fragment=std::string(remainder.substr(1));remainder={};
  }
  if(!remainder.empty())throw std::invalid_argument("unparsed URL suffix");
  // We deliberately reject non-ASCII host bytes instead of guessing IDNA mappings.
  for(unsigned char c:out.authority)if(c>=128)throw std::invalid_argument("non-ASCII host requires IDNA adapter");
  return out;
}
} // namespace net_detail
inline std::string percent_encode_rfc3986_component(std::string_view s) {return net_detail::encode(s,false);}
inline std::string encode_uri_component(std::string_view s) {return net_detail::encode(s,true);}
inline std::string decode_uri_component(std::string_view s) {return net_detail::decode(s);}
inline std::string join_url(std::string_view base,const std::vector<std::string_view>& parts) {
  const auto end=base.find_last_not_of('/');
  std::string result=end==std::string_view::npos?std::string{}:std::string(base.substr(0,end+1));
  for(auto part:parts){
    const auto first=part.find_first_not_of('/');
    if(first==std::string_view::npos)continue;
    const auto last=part.find_last_not_of('/');part=part.substr(first,last-first+1);
    if(!result.empty())result+='/';
    result.append(part);
  }
  return result;
}
inline bool is_url(std::string_view candidate) noexcept {
  try{(void)net_detail::parse_absolute_url(candidate);return true;}
  catch(...){return false;}
}
inline std::optional<std::string> get_query_param(std::string_view url,std::string_view key) {
  const auto parsed=net_detail::parse_absolute_url(url);
  if(!parsed.query_present)return std::nullopt;
  std::string_view query(parsed.query);
  for(std::size_t i=0;i<=query.size();){
    const auto end=query.find('&',i);
    const auto part=query.substr(i,end==std::string_view::npos?std::string_view::npos:end-i);
    const auto equals=part.find('=');
    if(net_detail::decode(part.substr(0,equals),true)==key)
      return net_detail::decode(equals==std::string_view::npos?std::string_view{}:part.substr(equals+1),true);
    if(end==std::string_view::npos)break;
    i=end+1;
  }
  return std::nullopt;
}
inline std::string set_query_param(std::string_view url,std::string_view key,std::string_view value) {
  auto parsed=net_detail::parse_absolute_url(url);
  (void)utf8_scalars(key);(void)utf8_scalars(value);
  std::vector<std::string> items;
  bool replaced=false;
  const std::string_view query(parsed.query);
  if(parsed.query_present&&!query.empty()){
    for(std::size_t i=0;i<=query.size();){
      const auto end=query.find('&',i);
      const auto part=query.substr(i,end==std::string_view::npos?std::string_view::npos:end-i);
      const auto equals=part.find('=');
      if(net_detail::decode(part.substr(0,equals),true)==key){
        if(!replaced){items.push_back(net_detail::encode(key,false,true)+"="+net_detail::encode(value,false,true));replaced=true;}
      }else items.emplace_back(part);
      if(end==std::string_view::npos)break;
    i=end+1;
    }
  }
  if(!replaced)items.push_back(net_detail::encode(key,false,true)+"="+net_detail::encode(value,false,true));
  parsed.query.clear();for(const auto& part:items){if(!parsed.query.empty())parsed.query+='&';parsed.query+=part;}
  parsed.query_present=true;
  return parsed.serialized();
}
inline std::string resolve_trusted_api_url(std::string_view origin,std::string_view path) {
  const auto base=net_detail::parse_absolute_url(origin);
  if(base.scheme!="https"||base.path!="/"||base.query_present||base.fragment_present
    ||origin.find('@')!=std::string_view::npos||origin.find('\\')!=std::string_view::npos)
    throw std::invalid_argument("trusted API origin must be a bare HTTPS origin");
  // A trailing slash is valid, but no other origin path or authority decorations.
  if(path.empty()||path[0]!='/'||(path.size()>=2&&path[1]=='/'))
    throw std::invalid_argument("only origin-relative API paths permitted");
  if(path.find('#')!=std::string_view::npos)throw std::invalid_argument("URL fragments forbidden");
  const auto query_start=path.find('?');
  const auto pathname=path.substr(0,query_start);
  for(unsigned char c:pathname)if(c=='\\'||c=='#'||c<32||c==127)
    throw std::invalid_argument("unsafe API path");
  for(std::size_t start=1;start<pathname.size();){
    const auto end=pathname.find('/',start);
    const auto segment=pathname.substr(start,end==std::string_view::npos?std::string_view::npos:end-start);
    if(!segment.empty())net_detail::validate_segment(std::string(segment),true);
    if(end==std::string_view::npos)break;
    start=end+1;
  }
  auto result=base.origin()+std::string(path);
  return result;
}
} // namespace gut
