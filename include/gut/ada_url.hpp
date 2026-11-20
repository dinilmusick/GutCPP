#pragma once
// Optional WHATWG URL API backed by pinned Ada URL C++ (the parser used by
// Node.js). No Node.js or browser dependency. Include only when Ada is linked.
#include <ada.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace gut {
namespace ada_detail {
inline unsigned hex_value(char c) noexcept {
  if(c>='0'&&c<='9')return static_cast<unsigned>(c-'0');
  if(c>='A'&&c<='F')return static_cast<unsigned>(c-'A'+10);
  if(c>='a'&&c<='f')return static_cast<unsigned>(c-'a'+10);
  return 256;
}
// The URLSearchParams constructor performs form decoding (including '+'),
// not generic URI decoding. Percent escapes are UTF-8 octets.
inline std::string form_decode(std::string_view raw) {
  std::string result;
  result.reserve(raw.size());
  for(std::size_t i=0;i<raw.size();++i){
    if(raw[i]=='+')result+=' ';
    else if(raw[i]=='%'&&i+2<raw.size()&&hex_value(raw[i+1])<16&&hex_value(raw[i+2])<16){
      result+=static_cast<char>((hex_value(raw[i+1])<<4)|hex_value(raw[i+2]));
      i+=2;
    }else result+=raw[i];
  }
  return result;
}
// application/x-www-form-urlencoded serialization is NOT RFC3986 encoding:
// U+0020 becomes '+', and punctuation such as '=' and '~' is escaped.
inline std::string form_encode(std::string_view value) {
  constexpr char hex[]="0123456789ABCDEF";
  std::string out;
  for(const unsigned char c:value){
    const bool allowed=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||
      (c>='0'&&c<='9')||c=='*'||c=='-'||c=='.'||c=='_';
    if(allowed)out+=static_cast<char>(c);
    else if(c==' ')out+='+';
    else {out+='%';out+=hex[c>>4];out+=hex[c&15];}
  }
  return out;
}
inline std::vector<std::pair<std::string,std::string>> query_pairs(std::string_view query){
  std::vector<std::pair<std::string,std::string>> pairs;
  for(std::size_t start=0;start<query.size();){
    auto end=query.find('&',start);
    if(end==std::string_view::npos)end=query.size();
    if(end>start){
      auto split=query.find('=',start);
      if(split==std::string_view::npos||split>=end)split=end;
      const auto key=form_decode(query.substr(start,split-start));
      const auto value=split==end?std::string{}:form_decode(query.substr(split+1,end-split-1));
      pairs.emplace_back(key,value);
    }
    start=end+1;
  }
  return pairs;
}
}
inline bool is_url_whatwg(std::string_view input) {
  return static_cast<bool>(ada::parse<ada::url_aggregator>(input));
}
inline std::optional<std::string> get_query_param_whatwg(
    std::string_view text, std::string_view key) {
  auto parsed=ada::parse<ada::url_aggregator>(text);
  if(!parsed)throw std::invalid_argument("Invalid absolute URL");
  auto query=parsed->get_search();
  if(!query.empty()&&query.front()=='?')query.remove_prefix(1);
  ada::url_search_params params(query);
  const auto value=params.get(key);
  if(!value)return std::nullopt;
  return std::string(*value);
}
inline std::string set_query_param_whatwg(std::string_view text,
    std::string_view key,std::string_view value) {
  auto parsed=ada::parse<ada::url_aggregator>(text);
  if(!parsed)throw std::invalid_argument("Invalid absolute URL");
  auto query=parsed->get_search();
  if(!query.empty()&&query.front()=='?')query.remove_prefix(1);
  auto pairs=ada_detail::query_pairs(query);
  bool replaced=false;
  std::vector<std::pair<std::string,std::string>> output;
  output.reserve(pairs.size()+1);
  for(auto& item:pairs){
    if(item.first==key){
      if(!replaced){output.emplace_back(std::move(item.first),std::string(value));replaced=true;}
    }else output.emplace_back(std::move(item));
  }
  if(!replaced)output.emplace_back(std::string(key),std::string(value));
  std::string search;
  for(const auto& item:output){
    if(!search.empty())search+='&';
    search+=ada_detail::form_encode(item.first);
    search+='=';
    search+=ada_detail::form_encode(item.second);
  }
  parsed->set_search(search);
  return std::string(parsed->get_href());
}
} // namespace gut
