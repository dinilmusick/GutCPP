#include "gut/icu_strings.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::cerr<<"failed line "<<__LINE__<<" "<<#x<<'\n';std::exit(1);}}while(false)
int main(){using namespace gut::icu_strings;
  CHECK(capitalize("FRED")=="Fred");
  CHECK(capitalize("foo BAR")=="Foo bar");
  CHECK(capitalize("\xC3\x89" "COLE")=="\xC3\x89" "cole");
  CHECK(capitalize("\xCE\xA3\xCE\x9C\xCE\x91")=="\xCE\xA3\xCE\xBC\xCE\xB1");
  CHECK(camel_case("Foo Bar")=="fooBar");
  CHECK(camel_case("HTMLParser Value")=="htmlParserValue");
  CHECK(camel_case("caf\xC3\xA9" "Cr\xC3\xA8me")=="cafeCreme");
  CHECK(kebab_case("FooBar BAZ")=="foo-bar-baz");
  CHECK(snake_case("FooBar BAZ")=="foo_bar_baz");
  CHECK(camel_case("a1B2")=="a1B2");
  CHECK(kebab_case("a1B2")=="a-1-b-2");
  CHECK(snake_case("a1B2")=="a_1_b_2");
  CHECK(camel_case("Αθήνα")=="αθήνα");
  CHECK(camel_case("🤖hello")=="🤖Hello");
  CHECK(kebab_case("smile 🙂 World")=="smile-🙂-world");
  CHECK(snake_case("👨‍👩‍👧‍👦 family")=="👨‍👩‍👧‍👦_family");

  CHECK(kebab_case("caf\xC3\xA9 Cr\xC3\xA8me")=="cafe-creme");
  CHECK(truncate("abcdefgh",5,"...")=="ab...");
  CHECK(truncate("abc",7,"...")=="abc");
  CHECK(truncate("abc",1,"long")=="long");
  CHECK(truncate("abc",0,"...")=="...");
  CHECK(truncate("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3" "abc",3,".")=="\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3" "a.");
  try{(void)truncate("a",-1);CHECK(false);}catch(const std::invalid_argument&){}
  try{(void)camel_case("\xC0\xAF");CHECK(false);}catch(const std::invalid_argument&){}
}
