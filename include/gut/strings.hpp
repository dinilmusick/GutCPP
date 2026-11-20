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

// Implements the default validator.js v13.15.15 isEmail() constraints used by
// GutTS@3e2cbc0, including UTF-8 local parts, quoted users and Unicode FQDNs.
// This intentionally does not implement validator.js non-default options.
namespace email_detail {
inline bool unicode_class(char32_t cp) noexcept {
  return (cp >= 0xA1 && cp <= 0xD7FF) ||
    (cp >= 0xF900 && cp <= 0xFDCF) ||
    (cp >= 0xFDF0 && cp <= 0xFFEF);
}
inline bool ascii_letter(char32_t cp) noexcept {
  return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z');
}
inline bool ascii_digit(char32_t cp) noexcept {return cp >= '0' && cp <= '9';}
inline bool user_char(char32_t cp) noexcept {
  constexpr std::string_view special = "!#$%&'*+-/=?^_`{|}~";
  return ascii_letter(cp) || ascii_digit(cp) ||
    (cp < 128 && special.find(static_cast<char>(cp)) != std::string_view::npos) ||
    unicode_class(cp);
}
inline bool quoted_char(char32_t cp) noexcept {
  // validator.js quotedEmailUserUtf8, matching its whitespace/control exceptions.
  return cp == ' ' || (cp >= 1 && cp <= 8) || (cp >= 9 && cp <= 13) ||
    (cp >= 14 && cp <= 31) || cp == 127 ||
    cp == '!' || (cp >= '#' && cp <= '[') ||
    (cp >= ']' && cp <= '~') || cp == 0xA0 || unicode_class(cp);
}
inline bool escaped_char(char32_t cp) noexcept {
  return (cp >= 1 && cp <= 9) || (cp >= 11 && cp <= 127) ||
    cp == 0xA0 || unicode_class(cp);
}
inline bool local_valid(std::string_view user) {
  if(user.empty() || user.size() > 64) return false;
  const auto codepoints=utf8_scalars(user);
  if(codepoints.size()>=2 && codepoints.front()=='"' && codepoints.back()=='"') {
    // A quoted local part can contain @, spaces, tabs and escaped double-quotes.
    for(std::size_t i=1;i+1<codepoints.size();++i){
      const auto cp=codepoints[i];
      if(cp=='\\'){
        if(i+2>=codepoints.size() || !escaped_char(codepoints[++i]))return false;
      }else if(!quoted_char(cp))return false;
    }
    return true;
  }
  // An unquoted mailbox is a sequence of nonempty dot-separated atom parts.
  bool atom=false;
  for(const char32_t cp:codepoints){
    if(cp=='.'){
      if(!atom)return false;
      atom=false;
    }else{
      if(!user_char(cp))return false;
      atom=true;
    }
  }
  return atom;
}
inline bool tld_char(char32_t cp) noexcept {
  return ascii_letter(cp) || (cp >= 0xA1 && cp <= 0xA8) ||
    cp == 0xAA || (cp >= 0xAB && cp <= 0xD7FF) ||
    (cp >= 0xF900 && cp <= 0xFDCF) ||
    (cp >= 0xFDF0 && cp <= 0xFFEF);
}
inline bool tld_valid(const std::vector<char32_t>& cps) noexcept {
  if(cps.size() >= 2 && std::all_of(cps.begin(),cps.end(),tld_char))return true;
  if(cps.size() >= 4 && (cps[0]=='x'||cps[0]=='X') && (cps[1]=='n'||cps[1]=='N')){
    for(std::size_t i=2;i<cps.size();++i)
      if(!ascii_letter(cps[i])&&!ascii_digit(cps[i])&&cps[i]!='-')return false;
    return true;
  }
  return false;
}
inline bool domain_valid(std::string_view domain) {
  if(domain.empty() || domain.size() > 254)return false;
  bool had_dot=false;
  std::vector<char32_t> last;
  for(std::size_t begin=0;begin<domain.size();){
    const std::size_t dot=domain.find('.',begin);
    const std::size_t end=dot==std::string_view::npos?domain.size():dot;
    const auto label=utf8_scalars(domain.substr(begin,end-begin));
    if(label.empty()||label.size()>63||label.front()=='-'||label.back()=='-')return false;
    for(const auto cp:label){
      if((cp >= 0xFF01 && cp <= 0xFF5E)||cp=='_'||
         !(ascii_letter(cp)||ascii_digit(cp)||cp=='-' || (cp >= 0xA1 && cp <= 0xFFFF)))
        return false;
    }
    last=label;
    if(dot==std::string_view::npos)break;
    had_dot=true;begin=dot+1;
    if(begin==domain.size())return false;
  }
  return had_dot && tld_valid(last);
}
inline bool email_valid(std::string_view text) {
  if(text.empty())return false;
  // JavaScript `String.length` counts UTF-16 code units, not UTF-8 bytes.
  std::size_t utf16=0;
  for(const char32_t cp:utf8_scalars(text))utf16+=cp>0xFFFF?2:1;
  if(utf16>254)return false;
  const auto at=text.rfind('@');
  if(at==std::string_view::npos||at==0||at+1==text.size())return false;
  const auto user=text.substr(0,at),domain=text.substr(at+1);
  return local_valid(user)&&domain_valid(domain);
}
} // namespace email_detail
inline bool is_email(std::string_view text) noexcept {
  try{return email_detail::email_valid(text);}catch(...){return false;}
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
