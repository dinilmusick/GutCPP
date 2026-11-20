#pragma once
// Optional production Unicode operations via ICU (not a core dependency).
#include "gut/portable.hpp"
#include <unicode/brkiter.h>
#include <unicode/normalizer2.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/locid.h>
#include <unicode/uscript.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gut::icu_strings {
namespace detail {
inline icu::UnicodeString decode(std::string_view input) {
  (void)utf8_scalars(input);
  return icu::UnicodeString::fromUTF8(icu::StringPiece(input.data(), static_cast<int32_t>(input.size())));
}
inline std::string encode(const icu::UnicodeString& input) {
  std::string result; input.toUTF8String(result); return result;
}
inline std::unique_ptr<icu::BreakIterator> characters() {
  UErrorCode status=U_ZERO_ERROR;
  std::unique_ptr<icu::BreakIterator> iterator(icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(),status));
  if(U_FAILURE(status)||!iterator)throw std::runtime_error("ICU grapheme segmentation unavailable");
  return iterator;
}
inline bool lower(UChar32 cp){return u_islower(cp);}
inline bool upper(UChar32 cp){return u_isupper(cp);}
inline bool letter_digit(UChar32 cp){return u_isalnum(cp);}
inline icu::UnicodeString deburr(const icu::UnicodeString& input) {
  UErrorCode status=U_ZERO_ERROR;
  const icu::Normalizer2* nfd=icu::Normalizer2::getNFDInstance(status);
  if(U_FAILURE(status))throw std::runtime_error("ICU normalization unavailable");
  icu::UnicodeString normalized;
  nfd->normalize(input,normalized,status);
  if(U_FAILURE(status))throw std::runtime_error("ICU normalization failed");
  icu::UnicodeString output;bool latin_base=false;
  for(int32_t i=0;i<normalized.length();) {
    const UChar32 cp=normalized.char32At(i);i+=U16_LENGTH(cp);
    if(cp==0x00DF){output.append("ss");latin_base=true;continue;}
    if(cp==0x00C6){output.append("Ae");latin_base=true;continue;}
    if(cp==0x00E6){output.append("ae");latin_base=true;continue;}
    if(cp==0x00D8){output.append('O');latin_base=true;continue;}
    if(cp==0x00F8){output.append('o');latin_base=true;continue;}
    if(cp==0x0141){output.append('L');latin_base=true;continue;}
    if(cp==0x0142){output.append('l');latin_base=true;continue;}
    if(cp==0x00DE){output.append("Th");latin_base=true;continue;}
    if(cp==0x00FE){output.append("th");latin_base=true;continue;}
    if(u_getCombiningClass(cp)!=0){if(latin_base)continue;}
    else {
      UErrorCode script_status=U_ZERO_ERROR;
      latin_base=uscript_getScript(cp,&script_status)==USCRIPT_LATIN&&U_SUCCESS(script_status);
    }
    output.append(cp);
  }
  // Keep non-Latin accents composed after Latin-only deburring.
  const icu::Normalizer2* nfc=icu::Normalizer2::getNFCInstance(status);
  if(U_FAILURE(status))throw std::runtime_error("ICU NFC normalization unavailable");
  icu::UnicodeString recomposed;
  nfc->normalize(output,recomposed,status);
  if(U_FAILURE(status))throw std::runtime_error("ICU NFC normalization failed");
  return recomposed;
}
inline std::vector<icu::UnicodeString> words(const icu::UnicodeString& input) {
  // Grapheme clusters preserve emoji flags, ZWJ joins and combining accents.
  const auto source=deburr(input);
  auto breaker=characters();breaker->setText(source);
  struct fragment {
    icu::UnicodeString text;
    UChar32 base=0;
    bool emoji=false,valid=false,digit=false;
  };
  std::vector<fragment> clusters;
  for(int32_t start=breaker->first(),end=breaker->next();end!=icu::BreakIterator::DONE;start=end,end=breaker->next()){
    auto piece=source.tempSubStringBetween(start,end);
    const auto cp=piece.char32At(0);
    const bool emoji=(cp>=0x1F1E6&&cp<=0x1F1FF)||u_hasBinaryProperty(cp,UCHAR_EXTENDED_PICTOGRAPHIC);
    clusters.push_back({std::move(piece),cp,emoji,emoji||letter_digit(cp),u_isdigit(cp)!=0});
  }
  std::vector<icu::UnicodeString> result;
  icu::UnicodeString current;
  bool prev_emoji=false,prev_digit=false;
  UChar32 prev_base=0;
  for(std::size_t i=0;i<clusters.size();++i){
    const auto& unit=clusters[i];
    if(!unit.valid){if(!current.isEmpty()){result.push_back(current);current.remove();}continue;}
    const UChar32 next=i+1<clusters.size()?clusters[i+1].base:0;
    const bool boundary=!current.isEmpty()&&(
      prev_emoji!=unit.emoji ||
      (!unit.emoji && prev_digit!=unit.digit) ||
      (!unit.emoji && !unit.digit && !prev_digit &&
        ((lower(prev_base)&&upper(unit.base)) ||
         (upper(prev_base)&&upper(unit.base)&&lower(next))))
    );
    if(boundary){result.push_back(current);current.remove();}
    current.append(unit.text);
    prev_emoji=unit.emoji;prev_digit=unit.digit;prev_base=unit.base;
  }
  if(!current.isEmpty())result.push_back(current);
  return result;
}
inline std::string join_words(std::string_view input,char separator,bool camel) {
  const auto parts=words(decode(input));icu::UnicodeString result;
  for(std::size_t i=0;i<parts.size();++i){
    auto word=parts[i];word.toLower(icu::Locale::getRoot());
    if(i&&!camel)result.append(static_cast<UChar>(separator));
    if(i&&camel){
      const UChar32 first=word.char32At(0);
      icu::UnicodeString lead(first);lead.toUpper(icu::Locale::getRoot());
      result.append(lead);result.append(word.tempSubString(U16_LENGTH(first)));
    }else result.append(word);
  }
  return encode(result);
}
} // namespace detail

inline std::string capitalize(std::string_view input) {
  auto u=detail::decode(input);if(u.isEmpty())return {};
  u.toLower(icu::Locale::getRoot());
  auto breaker=detail::characters();breaker->setText(u);
  const int32_t end=breaker->following(0);
  if(end==icu::BreakIterator::DONE)return detail::encode(u);
  auto first=u.tempSubStringBetween(0,end);first.toUpper(icu::Locale::getRoot());
  return detail::encode(first+u.tempSubString(end));
}
inline std::string camel_case(std::string_view input){return detail::join_words(input,'\0',true);}
inline std::string kebab_case(std::string_view input){return detail::join_words(input,'-',false);}
inline std::string snake_case(std::string_view input){return detail::join_words(input,'_',false);}
inline std::string truncate(std::string_view text,int64_t max_length,std::string_view omission="\xE2\x80\xA6"){
  if(max_length<0)throw std::invalid_argument("maxLength must be non-negative");
  auto input=detail::decode(text);auto ending=detail::decode(omission);
  auto breaker=detail::characters();
  const auto collect=[&](const icu::UnicodeString& str){
    breaker->setText(str);std::vector<int32_t> boundaries{0};
    for(int32_t n=breaker->next();n!=icu::BreakIterator::DONE;n=breaker->next())boundaries.push_back(n);
    return boundaries;
  };
  const auto units=collect(input);
  if(static_cast<int64_t>(units.size()-1)<=max_length)return std::string(text);
  const auto suffix_units=collect(ending);
  const auto suffix_size=static_cast<int64_t>(suffix_units.size()-1);
  if(suffix_size>=max_length)return std::string(omission);
  const auto prefix=static_cast<std::size_t>(max_length-suffix_size);
  return detail::encode(input.tempSubStringBetween(0,units[prefix]))+std::string(omission);
}
} // namespace gut::icu_strings
