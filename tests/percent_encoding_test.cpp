#include "gut/percent_encoding.hpp"
#include <cassert>
#include <string>
int main() {
  using gut::percent_encode_component;
  assert(percent_encode_component("").empty());
  assert(percent_encode_component("AZaz09-._~") == "AZaz09-._~");
  assert(percent_encode_component("a b/c?d=e&f#x") == "a%20b%2Fc%3Fd%3De%26f%23x");
  assert(percent_encode_component("%+") == "%25%2B");
  assert(percent_encode_component(std::string("\0\x01\x7F\xFF", 4)) == "%00%01%7F%FF");
  assert(percent_encode_component("\xC3\xA9") == "%C3%A9");
}
