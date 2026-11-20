#pragma once
// Platform-independent C++17 UTF-8 string operations inspired by GutTS.
// Case conversion intentionally covers ASCII letters only: full Unicode casefolding
// requires a Unicode database and is not silently emulated.
#include "gut/portable.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gut {
namespace string_detail {
inline bool ascii_lower(char ch) noexcept {return ch >= 'a' && ch <= 'z';}
inline bool ascii_upper(char ch) noexcept {return ch >= 'A' && ch <= 'Z';}
inline bool ascii_alpha(char ch) noexcept {return ascii_lower(ch) || ascii_upper(ch);}
inline bool ascii_digit(char ch) noexcept {return ch >= '0' && ch <= '9';}
inline char lower(char ch) noexcept {return ascii_upper(ch) ? static_cast<char>(ch + ('a'-'A')) : ch;}
inline char upper(char ch) noexcept {return ascii_lower(ch) ? static_cast<char>(ch - ('a'-'A')) : ch;}
inline bool alnum(unsigned char ch) noexcept {return ch >= 128 || ascii_alpha(static_cast<char>(ch)) || ascii_digit(static_cast<char>(ch));}
inline std::vector<std::string_view> words(std::string_view input) {
  (void)utf8_scalars(input); // reject truncated, overlong, surrogate and out-of-range input
  std::vector<std::string_view> out;
  std::size_t start = std::string_view::npos;
  const auto push = [&](std::size_t end) {if(start != std::string_view::npos && end > start) out.emplace_back(input.substr(start,end-start));};
  for(std::size_t i=0;i<input.size();++i) {
    const auto ch=static_cast<unsigned char>(input[i]);
    if(!alnum(ch)) {push(i);start=std::string_view::npos;continue;}
    if(start == std::string_view::npos) {start=i;continue;}
    const auto prev=static_cast<unsigned char>(input[i-1]);
    const bool boundary=(ascii_lower(static_cast<char>(prev)) && ascii_upper(static_cast<char>(ch))) ||
      (ascii_upper(static_cast<char>(prev)) && ascii_upper(static_cast<char>(ch)) && i+1<input.size() &&
       ascii_lower(input[i+1]));
    if(boundary) {push(i);start=i;}
  }
  push(input.size());
  return out;
}
inline std::string format_words(std::string_view text,char delimiter,bool camel) {
  const auto parts=words(text);
  std::string out;
  for(std::size_t i=0;i<parts.size();++i) {
    if(i && !camel) out+=delimiter;
    const auto part=parts[i];
    for(std::size_t j=0;j<part.size();++j) {
      char ch=string_detail::lower(part[j]);
      if(camel && i && !j) ch=string_detail::upper(ch);
      out+=ch;
    }
  }
  return out;
}
inline std::vector<std::string_view> scalar_slices(std::string_view text) {
  const auto cps=utf8_scalars(text);
  std::vector<std::string_view> out;out.reserve(cps.size());
  std::size_t i=0;
  for(char32_t cp:cps) {
    const std::size_t count=cp < 128 ? 1u : cp < 2048 ? 2u : cp < 65536 ? 3u : 4u;
    out.emplace_back(text.substr(i,count));i+=count;
  }
  return out;
}
} // namespace string_detail

// Conservative ASCII mailbox validator. Not a full validator.js isEmail port:
// deliberately rejects quoted mailbox, IDN domains, and addresses over 254 bytes.
inline bool is_email(std::string_view text) noexcept {
  if(text.empty()||text.size()>254) return false;
  const auto at=text.find('@');
  if(at==std::string_view::npos||at==0||at>64||at==text.size()-1||text.find('@',at+1)!=std::string_view::npos)return false;
  const auto local=text.substr(0,at),domain=text.substr(at+1);
  if(local.front()=='.'||local.back()=='.'||local.find("..")!=std::string_view::npos)return false;
  for(char c:local) if(!string_detail::ascii_alpha(c)&&!string_detail::ascii_digit(c)&&
       std::string_view("!#$%&'*+-/=?^_`{|}~.").find(c)==std::string_view::npos)return false;
  if(domain.empty()||domain.size()>253||domain.find('.')==std::string_view::npos)return false;
  for(std::size_t begin=0;begin<domain.size();) {
    const auto end=domain.find('.',begin);const auto length=(end==std::string_view::npos?domain.size():end)-begin;
    if(!length||length>63||domain[begin]=='-'||domain[begin+length-1]=='-')return false;
    for(std::size_t i=begin;i<begin+length;++i)if(!string_detail::ascii_alpha(domain[i]) &&
      !string_detail::ascii_digit(domain[i])&&domain[i]!='-')return false;
    if(end==std::string_view::npos)break;
    begin=end+1;
  }
  return true;
}
inline std::string capitalize(std::string_view text) {
  const auto chars=string_detail::scalar_slices(text);
  std::string out;out.reserve(text.size());
  for(std::size_t i=0;i<chars.size();++i){
    for(char ch:chars[i])out+=i==0?string_detail::upper(ch):string_detail::lower(ch);
  }
  return out;
}
inline std::string camel_case(std::string_view text){return string_detail::format_words(text,'\0',true);}
inline std::string kebab_case(std::string_view text){return string_detail::format_words(text,'-',false);}
inline std::string snake_case(std::string_view text){return string_detail::format_words(text,'_',false);}
inline std::string truncate(std::string_view text,std::ptrdiff_t max_length,std::string_view suffix="\xE2\x80\xA6") {
  if(max_length<0)throw std::invalid_argument("maxLength must be non-negative");
  const auto units=string_detail::scalar_slices(text);
  const auto ending=string_detail::scalar_slices(suffix);
  if(units.size()<=static_cast<std::size_t>(max_length))return std::string(text);
  if(ending.size()>=static_cast<std::size_t>(max_length))return std::string(suffix);
  std::string out;
  const auto retained=static_cast<std::size_t>(max_length)-ending.size();
  for(std::size_t i=0;i<retained;++i)out.append(units[i]);
  out.append(suffix);return out;
}
} // namespace gut
