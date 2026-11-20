#include "gut/strings.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"strings_test:"<<__LINE__<<": "<<#x<<"\n";std::exit(1);}}while(false)
int main(){
  using namespace gut;
  CHECK(is_email("alice+tag@example.com"));
  CHECK(is_email("user.name@sub.example.co.uk"));
  for(const auto& email:{"@example.com","bad@@example.com","foo..bar@example.com","a@b","x@-example.com","x@example..com","x@exam_ple.com"})CHECK(!is_email(email));
  CHECK(capitalize("FRED") == "Fred");
  CHECK(capitalize("foo BAR") == "Foo bar");
  CHECK(camel_case("Foo Bar") == "fooBar");
  CHECK(camel_case("HTMLParser Value") == "htmlParserValue");
  CHECK(kebab_case("FooBar BAZ") == "foo-bar-baz");
  CHECK(snake_case("FooBar BAZ") == "foo_bar_baz");
  CHECK(camel_case("hello---WORLD") == "helloWorld");
  CHECK(truncate("abcdefgh",5,"...") == "ab...");
  CHECK(truncate("abc",7,"...") == "abc");
  CHECK(truncate("abc",1,"long") == "long");
  CHECK(truncate("abc",0,"...") == "...");
  CHECK(truncate("caf\xC3\xA9\xF0\x9F\x98\x80",4,".") == "caf.");
  CHECK(truncate("caf\xC3\xA9\xF0\x9F\x98\x80",4,"...") == "c...");
  CHECK(capitalize("\xF0\x9F\x98\x80" "ABC") == "\xF0\x9F\x98\x80" "abc");
  try{(void)truncate("a",-1);CHECK(false);}catch(const std::invalid_argument&){}
  for(const auto& invalid:{"\xC0\xAF","\xED\xA0\x80","\xF4\x90\x80\x80"}){
    try{(void)camel_case(invalid);CHECK(false);}catch(const std::invalid_argument&){}
    try{(void)truncate(invalid,1);CHECK(false);}catch(const std::invalid_argument&){}
  }
}
