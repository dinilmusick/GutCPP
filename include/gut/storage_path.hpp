#pragma once
#include <stdexcept>
#include <string>
namespace gut {
namespace detail {
inline bool hex(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
inline unsigned nibble(char c) { return c >= '0' && c <= '9' ? unsigned(c-'0') : c >= 'a' && c <= 'f' ? unsigned(c-'a'+10) : unsigned(c-'A'+10); }
inline bool invalid(char c) { auto u = static_cast<unsigned char>(c); return c == '\\' || c == '?' || c == '#' || u < 32 || u == 127; }
inline bool escape(const std::string& s) { for (std::size_t i=0;i+2<s.size();++i) if(s[i]=='%' && hex(s[i+1]) && hex(s[i+2])) return true; return false; }
inline void utf8(const std::string& s) {
 for(std::size_t i=0;i<s.size();) {
  auto c=static_cast<unsigned char>(s[i]); if(c<128){++i;continue;}
  unsigned n=c>=0xC2&&c<=0xDF?2:c>=0xE0&&c<=0xEF?3:c>=0xF0&&c<=0xF4?4:0;
  if(!n||i+n>s.size()) throw std::invalid_argument("Invalid UTF-8");
  for(unsigned j=1;j<n;++j) if((static_cast<unsigned char>(s[i+j])&0xC0)!=0x80) throw std::invalid_argument("Invalid UTF-8");
  auto b=static_cast<unsigned char>(s[i+1]);
  if((c==0xE0&&b<0xA0)||(c==0xED&&b>=0xA0)||(c==0xF0&&b<0x90)||(c==0xF4&&b>=0x90)) throw std::invalid_argument("Invalid UTF-8");
  i+=n;
 }
}
inline std::string decode(const std::string& s) {
 std::string out; out.reserve(s.size());
 for(std::size_t i=0;i<s.size();++i) {
  if(s[i]!='%'){out+=s[i];continue;}
  if(i+2>=s.size()||!hex(s[i+1])||!hex(s[i+2])) throw std::invalid_argument("Invalid percent encoding");
  out+=static_cast<char>((nibble(s[i+1])<<4)|nibble(s[i+2])); i+=2;
 }
 utf8(out); return out;
}
inline void segment(std::string s) {
 for(unsigned depth=0;depth<8;++depth) {
  s=decode(s);
  if(s=="."||s==".."||s.find('/')!=std::string::npos) throw std::invalid_argument("Unsafe segment");
  for(char c:s) if(invalid(c)) throw std::invalid_argument("Unsafe segment");
  if(!escape(s)) return;
 }
 throw std::invalid_argument("Path too deeply encoded");
}
}
inline std::string normalize_storage_path(const std::string& path={}) {
 for(char c:path) if(detail::invalid(c)) throw std::invalid_argument("Invalid path");
 std::string out; std::size_t pos=0;
 while(pos<path.size()) {
  while(pos<path.size()&&path[pos]=='/') ++pos;
  if(pos==path.size()) break;
  auto end=path.find('/',pos);
  auto part=path.substr(pos,end==std::string::npos?end:end-pos);
  detail::segment(part);
  if(!out.empty()) out+='/';
  out+=part;
  if(end==std::string::npos) break;
  pos=end;
 }
 return out;
}
inline std::string encode_safe_path_segments(const std::string& path) {
 const auto normalized = normalize_storage_path(path);
 if(normalized.empty()) throw std::invalid_argument("A nonempty relative path is required");
 constexpr char digits[] = "0123456789ABCDEF";
 std::string encoded;
 for(unsigned char c : normalized) {
  if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
     (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
     c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' ||
     c == ')' || c == '/') encoded += static_cast<char>(c);
  else { encoded += '%'; encoded += digits[c >> 4]; encoded += digits[c & 15]; }
 }
 return encoded;
}
}
