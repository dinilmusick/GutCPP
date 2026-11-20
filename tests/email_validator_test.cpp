#include "gut/strings.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"email_validator_test line "<<__LINE__<<" "<<#x<<'\n';std::exit(1);}}while(false)
int main(){
  using gut::is_email;
  CHECK(is_email("alice+tag@example.com"));
  CHECK(is_email("\"a b\"@example.com"));
  CHECK(is_email("\"a@b\"@example.com"));
  CHECK(is_email("\"say\\\"hello\"@example.com"));
  CHECK(is_email("\"\"@example.com"));
  CHECK(is_email("a@e.xn--b"));
  CHECK(is_email("a@\xC3\xB6\xC3\xA4.example"));
  CHECK(is_email("caf\xC3\xA9@example.com"));
  CHECK(is_email("a@e.\xE4\xB8\xAD\xE6\x96\x87"));
  CHECK(!is_email("invalid"));
  CHECK(!is_email("name@localhost"));
  CHECK(!is_email("x@y.c"));
  CHECK(!is_email("x@y.123"));
  CHECK(!is_email("a..b@example.com"));
  CHECK(!is_email("@example.com"));
  CHECK(!is_email("user@-example.com"));
  CHECK(!is_email("user@example..com"));
  CHECK(!is_email("user@exam_ple.com"));
  CHECK(!is_email("user@example.com."));
  CHECK(!is_email("x@example.\xEF\xBC\xA1\xEF\xBC\xA2"));
  CHECK(!is_email("\xF0\x9F\x98\x80@example.com"));
  CHECK(!is_email("x@\xF0\x9F\x98\x80.example"));
  CHECK(!is_email("x@e.\xC0\xAF"));
  CHECK(!is_email("x@e.\xED\xA0\x80"));
  CHECK(!is_email(std::string(65,'x')+"@example.com"));
  CHECK(!is_email("name<user@example.com>"));
  CHECK(is_email(std::string(64,'x')+"@example.com"));
}
