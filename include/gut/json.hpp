#pragma once
// Strict, self-contained JSON value/parser/serializer. No external runtime.
#include "gut/portable.hpp"
#include <charconv>
#include <system_error>
#include <limits>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <variant>

namespace gut {
struct json_value {
  using array = std::vector<json_value>;
  using object = std::map<std::string,json_value>;
  using storage = std::variant<std::nullptr_t,bool,double,std::string,array,object>;
  storage data;
  json_value():data(nullptr){}
  json_value(std::nullptr_t):data(nullptr){}
  json_value(bool value):data(value){}
  json_value(double value):data(value){}
  json_value(int value):data(static_cast<double>(value)){}
  json_value(std::string value):data(std::move(value)){}
  json_value(const char* value):data(std::string(value)){}
  json_value(array value):data(std::move(value)){}
  json_value(object value):data(std::move(value)){}
  template<typename T> const T& as()const {return std::get<T>(data);}
  template<typename T> T& as(){return std::get<T>(data);}
  bool operator==(const json_value& other)const {return data==other.data;}
};
// Flatten one level of heterogeneous JSON arrays (unlike the homogeneous vector<T> helper).
inline json_value::array flatten_one(const json_value::array& values) {
  json_value::array out;
  for(const auto& value : values) {
    if(const auto* array=std::get_if<json_value::array>(&value.data))
      out.insert(out.end(),array->begin(),array->end());
    else out.push_back(value);
  }
  return out;
}
inline bool is_json_value(const json_value& value) {
  if(const auto* n=std::get_if<double>(&value.data)) return std::isfinite(*n);
  if(const auto* s=std::get_if<std::string>(&value.data)) {
    try{(void)utf8_scalars(*s);}catch(const std::invalid_argument&){return false;}
  }
  if(const auto* a=std::get_if<json_value::array>(&value.data))
    return std::all_of(a->begin(),a->end(),is_json_value);
  if(const auto* o=std::get_if<json_value::object>(&value.data)) {
    for(const auto& e:*o){
      try{(void)utf8_scalars(e.first);}catch(const std::invalid_argument&){return false;}
      if(!is_json_value(e.second)) return false;
    }
  }
  return true;
}
namespace json_detail {
inline void append_utf8(std::string& target,std::uint32_t cp) {
  if(cp<=0x7F) target+=static_cast<char>(cp);
  else if(cp<=0x7FF){target+=static_cast<char>(0xC0|(cp>>6));target+=static_cast<char>(0x80|(cp&63));}
  else if(cp<=0xFFFF){target+=static_cast<char>(0xE0|(cp>>12));target+=static_cast<char>(0x80|((cp>>6)&63));target+=static_cast<char>(0x80|(cp&63));}
  else {target+=static_cast<char>(0xF0|(cp>>18));target+=static_cast<char>(0x80|((cp>>12)&63));target+=static_cast<char>(0x80|((cp>>6)&63));target+=static_cast<char>(0x80|(cp&63));}
}
class parser {
  std::string_view input_;
  std::size_t pos_=0;
  void ws(){while(pos_<input_.size()&&(input_[pos_]==' '||input_[pos_]=='\n'||input_[pos_]=='\r'||input_[pos_]=='\t'))++pos_;}
  char get(){if(pos_>=input_.size())throw std::invalid_argument("truncated JSON");return input_[pos_++];}
  bool consume(char c){if(pos_<input_.size()&&input_[pos_]==c){++pos_;return true;}return false;}
  std::uint32_t hex_quad(){std::uint32_t n=0;for(int k=0;k<4;++k){const char c=get();n<<=4;
      if(c>='0'&&c<='9')n|=static_cast<std::uint32_t>(c-'0');
      else if(c>='a'&&c<='f')n|=static_cast<std::uint32_t>(c-'a'+10);
      else if(c>='A'&&c<='F')n|=static_cast<std::uint32_t>(c-'A'+10);
      else throw std::invalid_argument("invalid JSON Unicode escape");}return n;}
  std::string str(){if(get()!='"') throw std::invalid_argument("expected JSON string");
    std::string result;
    while(true){char c=get();if(c=='"'){(void)utf8_scalars(result);return result;}
      if(static_cast<unsigned char>(c)<0x20)throw std::invalid_argument("control byte in JSON string");
      if(c!='\\'){result+=c;continue;}
      switch(get()){
        case '"':result+='"';break; case '\\':result+='\\';break;
        case '/':result+='/';break;case 'b':result+='\b';break;
        case 'f':result+='\f';break;case 'n':result+='\n';break;
        case 'r':result+='\r';break;case 't':result+='\t';break;
        case 'u':{
          auto cp=hex_quad();
          if(cp>=0xD800&&cp<=0xDBFF){if(get()!='\\'||get()!='u')throw std::invalid_argument("missing Unicode surrogate pair");
            const auto low=hex_quad();if(low<0xDC00||low>0xDFFF)throw std::invalid_argument("invalid surrogate pair");
            cp=0x10000+((cp-0xD800)<<10)+(low-0xDC00);
          }else if(cp>=0xDC00&&cp<=0xDFFF)throw std::invalid_argument("lone low surrogate");
          append_utf8(result,cp);break;}
        default:throw std::invalid_argument("invalid JSON string escape");
      }
    }
  }
  json_value num(){const auto start=pos_;consume('-');
    if(consume('0')){if(pos_<input_.size()&&input_[pos_]>='0'&&input_[pos_]<='9')throw std::invalid_argument("JSON leading zero");}
    else {if(pos_==input_.size()||input_[pos_]<'1'||input_[pos_]>'9')throw std::invalid_argument("invalid JSON number");
      while(pos_<input_.size()&&input_[pos_]>='0'&&input_[pos_]<='9')++pos_;}
    if(consume('.')){const auto before=pos_;while(pos_<input_.size()&&input_[pos_]>='0'&&input_[pos_]<='9')++pos_;if(before==pos_)throw std::invalid_argument("invalid JSON fractional part");}
    if(consume('e')||consume('E')){if(!consume('+'))consume('-');const auto before=pos_;
      while(pos_<input_.size()&&input_[pos_]>='0'&&input_[pos_]<='9')++pos_;
      if(before==pos_)throw std::invalid_argument("invalid JSON exponent");}
    // Locale-independent numeric parsing. std::istream extraction differs
    // across libc++, libstdc++ and MSVC when a finite decimal underflows.
    const auto lexeme=input_.substr(start,pos_-start);
    double value=0;
    const auto conversion=std::from_chars(lexeme.data(),lexeme.data()+lexeme.size(),
                                          value,std::chars_format::general);
    if(conversion.ec==std::errc{}&&conversion.ptr==lexeme.data()+lexeme.size())
      return json_value(value);
    if(conversion.ec!=std::errc::result_out_of_range)
      throw std::invalid_argument("invalid JSON number");

    // Both overflows and underflows report result_out_of_range. Classify the
    // validated decimal token by its first significant digit's decimal order.
    std::size_t mantissa_end=lexeme.find_first_of("eE");
    if(mantissa_end==std::string_view::npos)mantissa_end=lexeme.size();
    std::int64_t exp10=0;
    if(mantissa_end<lexeme.size()){
      std::size_t at=mantissa_end+1;
      bool negative_exp=false;
      if(at<lexeme.size()&&(lexeme[at]=='+'||lexeme[at]=='-')){
        negative_exp=lexeme[at]=='-';++at;
      }
      for(;at<lexeme.size();++at)
        exp10=std::min<std::int64_t>(1000000,exp10*10+(lexeme[at]-'0'));
      if(negative_exp)exp10=-exp10;
    }
    std::int64_t integer_digits=0,first_nonzero=-1,total_digits=0;
    bool dot=false;
    for(std::size_t at=lexeme[0]=='-'?1u:0u;at<mantissa_end;++at){
      const char ch=lexeme[at];
      if(ch=='.'){dot=true;continue;}
      if(!dot)++integer_digits;
      if(first_nonzero<0&&ch!='0')first_nonzero=total_digits;
      ++total_digits;
    }
    if(first_nonzero<0)return json_value(lexeme[0]=='-'?-0.0:0.0);
    const std::int64_t magnitude=integer_digits-first_nonzero-1+exp10;
    if(magnitude < -307)return json_value(lexeme[0]=='-'?-0.0:0.0);
    if(magnitude >= 308)return json_value(lexeme[0]=='-'?-std::numeric_limits<double>::infinity():std::numeric_limits<double>::infinity());
    throw std::invalid_argument("floating conversion outside representable range");
  }
  json_value parse(unsigned depth){if(depth>=128)throw std::invalid_argument("JSON nesting limit exceeded");ws();
    if(pos_>=input_.size())throw std::invalid_argument("empty JSON");
    if(input_[pos_]=='"')return json_value(str());
    if(consume('[')){
      json_value::array out;ws();if(consume(']'))return json_value(std::move(out));
      do{out.push_back(parse(depth+1));ws();if(consume(']'))break;if(!consume(','))throw std::invalid_argument("invalid JSON array separator");}
      while(true);return json_value(std::move(out));
    }
    if(consume('{')){
      json_value::object out;ws();if(consume('}'))return json_value(std::move(out));
      do{ws();if(pos_>=input_.size()||input_[pos_]!='"')throw std::invalid_argument("invalid JSON object key");
        auto key=str();ws();if(!consume(':'))throw std::invalid_argument("missing JSON object colon");
        out.insert_or_assign(std::move(key),parse(depth+1));ws();if(consume('}'))break;
        if(!consume(','))throw std::invalid_argument("invalid JSON object separator");}
      while(true);return json_value(std::move(out));
    }
    for(auto kv:{std::pair<std::string_view,json_value>{"true",json_value(true)},
                 {"false",json_value(false)},{"null",json_value(nullptr)}}) {
      if(input_.substr(pos_,kv.first.size())==kv.first){pos_+=kv.first.size();return kv.second;}
    }
    if(input_[pos_]=='-'||(input_[pos_]>='0'&&input_[pos_]<='9'))return num();
    throw std::invalid_argument("unexpected JSON token");
  }
 public:
  explicit parser(std::string_view input):input_(input){}
  json_value document(){auto result=parse(0);ws();if(pos_!=input_.size())throw std::invalid_argument("trailing JSON text");return result;}
};
inline std::string escaped(std::string_view s){(void)utf8_scalars(s);std::string out="\"";
  constexpr char digits[]="0123456789abcdef";
  for(unsigned char c:s){
    switch(c){case '"':out+="\\\"";break;case '\\':out+="\\\\";break;
      case '\b':out+="\\b";break;case '\f':out+="\\f";break;
      case '\n':out+="\\n";break;case '\r':out+="\\r";break;
      case '\t':out+="\\t";break;
      default:if(c<32){out+="\\u00";out+=digits[c>>4];out+=digits[c&15];}
        else {out+=static_cast<char>(c);}
        break;}
  }
  return out+'"';}
// C++17 shortest roundtrip numeric digits with ECMAScript exponent thresholds.
inline std::string ecmascript_number(double number) {
  if(!std::isfinite(number))return "null";
  if(number==0.0)return "0";
  char buffer[128];
  const auto result=std::to_chars(buffer,buffer+sizeof(buffer),number,std::chars_format::general);
  if(result.ec!=std::errc{})throw std::invalid_argument("JSON number conversion failed");
  std::string repr(buffer,result.ptr);
  std::string sign;
  if(repr.front()=='-'){sign="-";repr.erase(0,1);}
  const auto exp_at=repr.find_first_of("eE");
  int exponent=0;
  if(exp_at!=std::string::npos){
    const auto exponent_text=repr.substr(exp_at+1);
    const char* first=exponent_text.data();
    if(!exponent_text.empty()&&exponent_text.front()=='+')++first;
    const auto parsed=std::from_chars(first,exponent_text.data()+exponent_text.size(),exponent);
    if(parsed.ec!=std::errc{}||parsed.ptr!=exponent_text.data()+exponent_text.size())
      throw std::invalid_argument("invalid decimal exponent from compiler");
    repr.resize(exp_at);
  }
  const auto dot=repr.find('.');
  int decimal_point=static_cast<int>(dot==std::string::npos?repr.size():dot)+exponent;
  if(dot!=std::string::npos)repr.erase(dot,1);
  std::size_t zeroes=0;
  while(zeroes+1<repr.size()&&repr[zeroes]=='0')++zeroes;
  repr.erase(0,zeroes);
  decimal_point-=static_cast<int>(zeroes);
  while(repr.size()>1&&repr.back()=='0')repr.pop_back();
  const int decimal_exponent=decimal_point-1;
  if(decimal_exponent>=-6&&decimal_exponent<21){
    if(decimal_point<=0)return sign+"0."+std::string(static_cast<std::size_t>(-decimal_point),'0')+repr;
    if(decimal_point>=static_cast<int>(repr.size()))
      return sign+repr+std::string(static_cast<std::size_t>(decimal_point-static_cast<int>(repr.size())),'0');
    repr.insert(static_cast<std::size_t>(decimal_point),1,'.');
    return sign+repr;
  }
  std::string out=sign+repr.substr(0,1);
  if(repr.size()>1)out+='.'+repr.substr(1);
  out+='e';
  if(decimal_exponent>=0)out+='+';
  out+=std::to_string(decimal_exponent);
  return out;
}
inline void render(const json_value& value,std::string& out,unsigned depth,unsigned space){
  if(depth>=128)throw std::invalid_argument("JSON nesting limit exceeded");
  if(std::holds_alternative<std::nullptr_t>(value.data)) {out+="null";return;}
  if(const auto b=std::get_if<bool>(&value.data)){out+=*b?"true":"false";return;}
  if(const auto n=std::get_if<double>(&value.data)){
    out+=ecmascript_number(*n);return;
  }
  if(const auto s=std::get_if<std::string>(&value.data)){out+=escaped(*s);return;}
  const auto indent=[&](unsigned d){if(space){out+='\n';out.append(static_cast<std::size_t>(d)*space,' ');}};
  if(const auto a=std::get_if<json_value::array>(&value.data)){
    out+='[';bool first=true;for(const auto& v:*a){if(!first)out+=',';indent(depth+1);render(v,out,depth+1,space);first=false;}
    if(!a->empty()) {indent(depth);}
    out+=']';return;
  }
  const auto& o=std::get<json_value::object>(value.data);
  out+='{';bool first=true;for(const auto& [key,v]:o){if(!first)out+=',';indent(depth+1);
    out+=escaped(key);out+=space?": ":":";render(v,out,depth+1,space);first=false;}
  if(!o.empty()) {indent(depth);}
  out+='}';
}
} // namespace json_detail
inline json_value parse_json(std::string_view text){return json_detail::parser(text).document();}
inline std::optional<json_value> try_parse_json(std::string_view text) noexcept {
  try{return parse_json(text);}catch(...){return std::nullopt;}
}
inline std::string stringify_json(const json_value& value,unsigned space=0){
  // stringifyJson emits null for nonfinite numbers, even though
  // isJsonValue correctly reports them outside the JSON value domain.
  // Every string and object key is validated by escaped().
  std::string output;json_detail::render(value,output,0,std::min(space,10u));return output;
}
} // namespace gut
