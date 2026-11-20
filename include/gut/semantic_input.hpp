#pragma once
// Deterministic portable portion of pinned GutTS semantic-input resolver.
// ML-assisted extraction and ranking remain separate real-model operations.
#include "gut/json.hpp"
#include "gut/portable.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <list>
#include <locale>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gut {
struct semantic_input_schema {
  std::string type;
  std::optional<std::string> format;
  std::optional<std::string> pattern;
  std::vector<json_value> allowed;
};
struct semantic_input_resolution {
  std::string status;
  std::optional<json_value> value;
  std::string raw_input;
  bool requires_confirmation=false;
};
namespace semantic_detail {
inline std::string trim(std::string_view input) {
  const auto chars=utf8_scalars(input);
  std::size_t first=0,last=chars.size();
  while(first<last&&is_js_whitespace(chars[first]))++first;
  while(last>first&&is_js_whitespace(chars[last-1]))--last;
  std::size_t begin=0,end=0;
  for(std::size_t i=0;i<chars.size();++i){
    const auto cp=chars[i];const std::size_t width=cp<0x80?1u:cp<0x800?2u:cp<0x10000?3u:4u;
    if(i<first)begin+=width;
    if(i<last)end+=width;
  }
  return std::string(input.substr(begin,end-begin));
}
inline bool decimal(std::string_view input,double& number) {
  if(input.empty())return false;
  std::istringstream stream{std::string(input)};
  stream.imbue(std::locale::classic());stream>>std::noskipws>>number;
  return !stream.fail() && stream.eof() && std::isfinite(number);
}
inline bool re_match(std::string_view value,const std::string& pattern) {
  // Caller-supplied regexes are a potential denial-of-service vector.
  // Expose a bounded simple path; no unbounded regex execution here.
  if(pattern.size()>128||value.size()>2048) return false;
  try {const std::regex expression(pattern,std::regex::ECMAScript|std::regex::optimize);
    return std::regex_search(value.begin(),value.end(),expression);}
  catch(const std::regex_error&){return false;}
}
inline bool looks_like_email(std::string_view input) {
  if(input.find(' ')!=std::string_view::npos)return false;
  const auto at=input.find('@');
  return at!=std::string_view::npos&&at>0&&at<input.size()-1&&input.find('@',at+1)==std::string_view::npos
    &&input.find('.',at+1)!=std::string_view::npos;
}
inline bool prefixed(std::string_view value,std::string_view prefix) noexcept {return value.substr(0,prefix.size())==prefix;}
inline bool date_prefix(std::string_view value) noexcept {
  if(value.size()<10||value[4]!='-'||value[7]!='-')return false;
  for(std::size_t i=0;i<10;++i)if(i!=4&&i!=7&&(value[i]<'0'||value[i]>'9'))return false;
  return true;
}
inline std::string ascii_lower(std::string_view input){
  std::string out(input);for(auto& c:out)if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));return out;
}
}
inline std::optional<semantic_input_resolution> resolve_deterministic_semantic_input(
    std::string_view raw,const semantic_input_schema& schema) {
  const auto input=semantic_detail::trim(raw);
  if(input.empty())return std::nullopt;
  const json_value text(input);
  if(std::find(schema.allowed.begin(),schema.allowed.end(),text)!=schema.allowed.end())
    return semantic_input_resolution{"resolved",text,std::string(raw),false};
  if(schema.type=="string") {
    if(schema.format) {
      if(*schema.format=="email"&&!semantic_detail::looks_like_email(input))return std::nullopt;
      if(*schema.format=="uri"&&!semantic_detail::prefixed(input,"http://")&&!semantic_detail::prefixed(input,"https://"))return std::nullopt;
      if(*schema.format=="date"&&(!semantic_detail::date_prefix(input)||input.size()!=10))return std::nullopt;
      if(*schema.format=="date-time"&&(!semantic_detail::date_prefix(input)||input.size()<11||input[10]!='T'))return std::nullopt;
    }
    if(schema.pattern&&!semantic_detail::re_match(input,*schema.pattern))return std::nullopt;
    if(!schema.format&&!schema.pattern)return std::nullopt; // TS does not deterministically resolve generic strings
    return semantic_input_resolution{"resolved",text,std::string(raw),false};
  }
  if(schema.type=="number"||schema.type=="integer") {
    double parsed=0;if(!semantic_detail::decimal(input,parsed))return std::nullopt;
    if(schema.type=="integer"&&std::trunc(parsed)!=parsed)return std::nullopt;
    return semantic_input_resolution{"resolved",json_value(parsed),std::string(raw),false};
  }
  if(schema.type=="boolean"){
    const auto lower=semantic_detail::ascii_lower(input);
    if(lower=="true"||lower=="false")
      return semantic_input_resolution{"resolved",json_value(lower=="true"),std::string(raw),false};
  }
  return std::nullopt;
}
// Synchronized LRU key/value store; no background agents or scheduling.
class semantic_input_cache {
  using entry=std::pair<std::string,semantic_input_resolution>;
  const std::size_t capacity_;
  mutable std::mutex mutex_;
  std::list<entry> entries_;
  std::unordered_map<std::string,typename std::list<entry>::iterator> index_;
 public:
  explicit semantic_input_cache(std::size_t max_entries=128):capacity_(max_entries){}
  semantic_input_cache(const semantic_input_cache&)=delete;
  semantic_input_cache& operator=(const semantic_input_cache&)=delete;
  std::optional<semantic_input_resolution> get(const std::string& key){
    std::lock_guard<std::mutex> lock(mutex_);const auto it=index_.find(key);
    if(it==index_.end())return std::nullopt;
    entries_.splice(entries_.end(),entries_,it->second);
    return it->second->second;
  }
  void set(std::string key,semantic_input_resolution value){
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it=index_.find(key);
    if(it!=index_.end()){entries_.erase(it->second);index_.erase(it);}
    if(!capacity_)return;
    entries_.emplace_back(std::move(key),std::move(value));
    auto last=std::prev(entries_.end());index_[last->first]=last;
    while(entries_.size()>capacity_){index_.erase(entries_.front().first);entries_.pop_front();}
  }
  void clear(){std::lock_guard<std::mutex> lock(mutex_);entries_.clear();index_.clear();}
  std::size_t size() const {std::lock_guard<std::mutex> lock(mutex_);return entries_.size();}
};
} // namespace gut
