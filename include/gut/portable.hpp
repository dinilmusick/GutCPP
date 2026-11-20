#pragma once
// Portable GutTS value operations. C++17; no environment or locale globals.
#include <algorithm>
#include <array>
#include <charconv>
#include <system_error>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace gut {

// Lodash compact discards JavaScript-falsy scalars (not just null/undefined).
// All present class and container values remain truthy, including empty ones.
template <class T> bool compact_truthy(const T& value) {
  if constexpr (std::is_same_v<T,bool>) return value;
  else if constexpr (std::is_arithmetic_v<T>) {
    if constexpr (std::is_floating_point_v<T>) return value != 0 && !std::isnan(value);
    else return value != 0;
  }
  else if constexpr (std::is_same_v<T,std::string>) return !value.empty();
  else return true;
}
template <class T> std::vector<T> compact(const std::vector<std::optional<T>>& source) {
  std::vector<T> result; result.reserve(source.size());
  for (const auto& v : source) if (v && compact_truthy(*v)) result.push_back(*v);
  return result;
}
template <class T> std::vector<T> unique(const std::vector<T>& source) {
  std::vector<T> result; result.reserve(source.size());
  for (const auto& v : source)
    if (std::find(result.begin(), result.end(), v) == result.end()) result.push_back(v);
  return result;
}
template <class T> std::vector<std::vector<T>> chunk(const std::vector<T>& source, std::ptrdiff_t size) {
  if (size<=0) throw std::invalid_argument("size must be a positive integer");
  std::vector<std::vector<T>> result;
  for (std::size_t begin = 0; begin < source.size();) {
    const auto count = std::min(static_cast<std::size_t>(size), source.size()-begin);
    result.emplace_back(source.begin()+static_cast<std::ptrdiff_t>(begin),
                        source.begin()+static_cast<std::ptrdiff_t>(begin+count));
    begin += count;
  }
  return result;
}
template <class T> std::vector<T> flatten(const std::vector<std::vector<T>>& source) {
  std::size_t total = 0;
  for (const auto& segment : source) {
    if (segment.size() > std::numeric_limits<std::size_t>::max()-total)
      throw std::length_error("flattened array size overflow");
    total += segment.size();
  }
  std::vector<T> result; result.reserve(total);
  for (const auto& segment : source) result.insert(result.end(), segment.begin(), segment.end());
  return result;
}
template <class T> std::optional<T> last(const std::vector<T>& source) {
  if (source.empty()) return std::nullopt;
  return source.back();
}
template <class T> bool has_own(const std::map<std::string,T>& object, std::string_view key) {
  return object.find(std::string(key)) != object.end();
}
template <class T> std::map<std::string,T> pick(const std::map<std::string,T>& source,
                                                  const std::vector<std::string>& keys) {
  std::map<std::string,T> out;
  for (const auto& k : keys) {const auto it=source.find(k); if(it!=source.end()) out.emplace(it->first,it->second);}
  return out;
}
template <class T> std::map<std::string,T> omit(const std::map<std::string,T>& source,
                                                  const std::vector<std::string>& keys) {
  auto out=source; for (const auto& k : keys) out.erase(k); return out;
}
inline double clamp(double value, double low, double high) {
  if (low>high) throw std::invalid_argument("min must not exceed max");
  return std::min(std::max(value,low),high);
}
inline bool in_range(double value,double low,double high) {
  if (low>high) throw std::invalid_argument("min must not exceed max");
  return value>=low && value<high; // lodash inRange has an EXCLUSIVE upper bound
}
inline bool is_finite_number(double value) noexcept {return std::isfinite(value);}
inline double sum(const std::vector<double>& values) noexcept {
  double result=0; for(double v:values) result+=v; return result;
}
namespace numeric_detail {
// Lodash shifts a decimal exponent rather than multiplying a floating-point
// number by 10^precision. 1.005*100 is imprecise; parsing "1.005e2" is not.
// Floating charconv is locale-independent and emits the shortest roundtrip
// decimal (as the JavaScript Number-to-string conversion normally does).
inline std::optional<double> shift_decimal_exponent(double number,int shift) {
  std::array<char,128> buffer{};
  const auto printed=std::to_chars(buffer.data(),buffer.data()+buffer.size(),number,std::chars_format::general);
  if(printed.ec!=std::errc{})return std::nullopt;
  const std::string_view digits(buffer.data(),static_cast<std::size_t>(printed.ptr-buffer.data()));
  const auto mark=digits.find_first_of("eE");
  int original_exponent=0;
  if(mark!=std::string_view::npos) {
    const auto start=digits.data()+mark+1;
    const auto parsed=std::from_chars(start,digits.data()+digits.size(),original_exponent);
    if(parsed.ec!=std::errc{}||parsed.ptr!=digits.data()+digits.size())return std::nullopt;
  }
  std::string scaled(digits.substr(0,mark));
  scaled+='e';scaled+=std::to_string(original_exponent+shift);
  double result=0.0;
  const auto parsed=std::from_chars(scaled.data(),scaled.data()+scaled.size(),result,std::chars_format::general);
  if(parsed.ec==std::errc::result_out_of_range&&original_exponent+shift<0)
    return std::copysign(0.0,number);
  if(parsed.ec!=std::errc{}||parsed.ptr!=scaled.data()+scaled.size())return std::nullopt;
  return result;
}
inline double js_round(double value) {
  if(!std::isfinite(value)||value==0.0)return value;
  const auto result=std::floor(value+0.5);
  return result==0.0&&std::signbit(value)?-0.0:result;
}
} // namespace numeric_detail
inline double round_to(double value,int decimals=0) {
  if(decimals<0)throw std::invalid_argument("decimals must be a non-negative integer");
  if(!std::isfinite(value)||value==0.0)return value;
  if(decimals==0)return numeric_detail::js_round(value);
  if(decimals>308)return value; // prevent exponent overflow; not a JS-number emulator
  const auto scaled=numeric_detail::shift_decimal_exponent(value,decimals);
  if(!scaled||!std::isfinite(*scaled))return value;
  const auto rounded=numeric_detail::js_round(*scaled);
  const auto restored=numeric_detail::shift_decimal_exponent(rounded,-decimals);
  return restored.value_or(value);
}

// Strict UTF-8 scalar decoder, rejects overlong sequences, surrogates and > U+10FFFF.
inline std::vector<char32_t> utf8_scalars(std::string_view s) {
  std::vector<char32_t> out; out.reserve(s.size());
  for(std::size_t i=0;i<s.size();) {
    const auto c=static_cast<unsigned char>(s[i]);
    if(c<0x80) {out.push_back(c);++i;continue;}
    unsigned n=0;char32_t cp=0;
    if(c>=0xC2&&c<=0xDF) {n=2;cp=c&0x1F;}
    else if(c>=0xE0&&c<=0xEF) {n=3;cp=c&0x0F;}
    else if(c>=0xF0&&c<=0xF4) {n=4;cp=c&0x07;}
    else throw std::invalid_argument("invalid UTF-8 leading byte");
    if(n>s.size()-i) throw std::invalid_argument("truncated UTF-8 sequence");
    for(unsigned k=1;k<n;++k) {
      const auto next=static_cast<unsigned char>(s[i+k]);
      if((next&0xC0)!=0x80) throw std::invalid_argument("invalid UTF-8 continuation");
      cp=(cp<<6)|(next&0x3F);
    }
    if((n==2 && cp<0x80)||(n==3 && cp<0x800)||(n==4 && cp<0x10000)
       ||(cp>=0xD800&&cp<=0xDFFF)||cp>0x10FFFF)
       throw std::invalid_argument("non-scalar UTF-8 sequence");
    out.push_back(cp);i+=n;
  }
  return out;
}
inline bool is_js_whitespace(char32_t cp) noexcept {
  return cp==0x09||cp==0x0A||cp==0x0B||cp==0x0C||cp==0x0D||cp==0x20
      ||cp==0xA0||cp==0x1680||(cp>=0x2000&&cp<=0x200A)
      ||cp==0x2028||cp==0x2029||cp==0x202F||cp==0x205F||cp==0x3000||cp==0xFEFF;
}
inline bool is_blank(std::string_view text) {
  const auto cps=utf8_scalars(text);
  return std::all_of(cps.begin(),cps.end(),is_js_whitespace);
}
inline bool is_non_blank(std::string_view text) {return !is_blank(text);}
inline bool starts_with_any(std::string_view text,const std::vector<std::string>& prefixes) {
  for(const auto& prefix:prefixes) if(text.substr(0,prefix.size())==prefix) return true;
  return false;
}
inline bool ends_with_any(std::string_view text,const std::vector<std::string>& suffixes) {
  for(const auto& suffix:suffixes)
    if(text.size()>=suffix.size()&&text.substr(text.size()-suffix.size())==suffix) return true;
  return false;
}
inline std::string truncate_words(std::string_view text,std::ptrdiff_t max_words,std::string_view suffix="\xE2\x80\xA6") {
  if(max_words<0)throw std::invalid_argument("maxWords must be non-negative");
  if(text.empty()) return {};
  const auto cps=utf8_scalars(text);
  std::vector<std::string_view> words;
  std::size_t offset=0;
  std::optional<std::size_t> word_begin;
  for(char32_t cp:cps) {
    const auto width=cp<0x80?1u:cp<0x800?2u:cp<0x10000?3u:4u;
    if(is_js_whitespace(cp)) {
      if(word_begin) {words.emplace_back(text.substr(*word_begin,offset-*word_begin));word_begin.reset();}
    } else if(!word_begin) word_begin=offset;
    offset+=width;
  }
  if(word_begin) words.emplace_back(text.substr(*word_begin,offset-*word_begin));
  if(words.size()<=static_cast<std::size_t>(max_words)) return std::string(text);
  std::string out;
  for(std::size_t i=0;i<static_cast<std::size_t>(max_words);++i) {if(i)out+=' ';out.append(words[i]);}
  out.append(suffix);return out;
}
inline bool is_identity_kind(std::string_view kind) noexcept {
  return kind=="application"||kind=="deployment"||kind=="oauth_client"
    ||kind=="user"||kind=="workspace"||kind=="ui_session";
}
inline bool valid_identity_id(std::string_view kind,std::string_view id) noexcept {
  if(!is_identity_kind(kind)||id.empty()||id.size()>256) return false;
  for(std::size_t i=0;i<id.size();++i) {
    const char ch=id[i];
    const bool alnum=(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9');
    if(!alnum && (i==0 || (ch!='.'&&ch!='_'&&ch!=':'&&ch!='-'))) return false;
  }
  return true;
}
struct identity_ref { std::string kind; std::string id; };
inline std::optional<identity_ref> parse_identity_ref(std::string kind,std::string id) {
  if(!valid_identity_id(kind,id))return std::nullopt;
  return identity_ref{std::move(kind),std::move(id)};
}
inline std::optional<std::string> parse_legacy_application_id(
  const std::map<std::string,std::string>& aliases) {
  std::optional<std::string> result;
  for(const auto key:{"app_id","appId","applicationId"}) {
    const auto it=aliases.find(key);
    if(it==aliases.end()) continue;
    if(!valid_identity_id("application",it->second)|| (result && *result!=it->second)) return std::nullopt;
    result=it->second;
  }
  return result;
}
} // namespace gut
