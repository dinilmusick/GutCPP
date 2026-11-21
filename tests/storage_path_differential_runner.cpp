#include "gut/storage_path.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
static int digit(char ch) {
  if(ch>='0'&&ch<='9') return ch-'0';
  if(ch>='a'&&ch<='f') return ch-'a'+10;
  if(ch>='A'&&ch<='F') return ch-'A'+10;
  return -1;
}
static std::string from_hex(const std::string& raw) {
  if(raw.size()%2) throw std::invalid_argument("odd hex input");
  std::string decoded;
  decoded.reserve(raw.size()/2);
  for(std::size_t i=0;i<raw.size();i+=2) {
    const int hi=digit(raw[i]),lo=digit(raw[i+1]);
    if(hi<0||lo<0) throw std::invalid_argument("invalid hex");
    decoded.push_back(static_cast<char>((hi<<4)|lo));
  }
  return decoded;
}
static std::string to_hex(const std::string& raw) {
  constexpr char alphabet[]="0123456789abcdef";
  std::string encoded;
  encoded.reserve(raw.size()*2);
  for(unsigned char c:raw) {
    encoded.push_back(alphabet[c>>4]);
    encoded.push_back(alphabet[c&15]);
  }
  return encoded;
}
int main(int argc,char**argv) {
  if(argc!=3) { std::cerr<<"usage: <normalize|encode> <utf8-hex>\n"; return 2; }
  try {
    const std::string kind=argv[1],input=from_hex(argv[2]);
    std::string result;
    if(kind=="normalize") result=gut::normalize_storage_path(input);
    else if(kind=="encode") result=gut::encode_safe_path_segments(input);
    else { std::cerr<<"unknown operation\n"; return 2; }
    std::cout<<"ok\t"<<to_hex(result)<<"\n";
    return 0;
  } catch(const std::exception&) {
    std::cout<<"error\n";
    return 0;
  }
}
