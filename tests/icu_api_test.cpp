#include "gut/api.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#define CHECK(x) do{if(!(x)){std::cerr<<"icu_api_test line "<<__LINE__<<" "<<#x<<'\n';std::exit(1);}}while(false)
static std::string call(const std::string& name, const std::string& value) {
  const auto output=gut::execute_gutts_api("gutts.strings."+name,
    gut::json_value::object{{"value",value}});
  CHECK(output.has_value());
  return output->as<std::string>();
}
int main() {
#ifndef GUTCPP_WITH_ICU_STRINGS
#error "The CMake opt-in gutcpp::icu target must define GUTCPP_WITH_ICU_STRINGS"
#endif
  CHECK(gut::find_portable_api("gutts.strings.camelCase")!=nullptr);
  CHECK(call("capitalize","FRED")=="Fred");
  CHECK(call("capitalize","ÉCOLE")=="École");
  CHECK(call("camelCase","café Crème")=="cafeCreme");
  CHECK(call("kebabCase","Mañana Señor")=="manana-senor");
  CHECK(call("snakeCase","FooBAR")=="foo_bar");
  const gut::json_value::object trunc{
    {"value","🇮🇳abc"},{"maxLength",3},{"suffix","."}
  };
  const auto t=gut::execute_gutts_api("gutts.strings.truncate",trunc);
  CHECK(t.has_value());
  CHECK(t->as<std::string>()=="🇮🇳a.");
  const auto email=gut::execute_gutts_api("gutts.strings.isEmail",
    gut::json_value::object{{"value","josé@example.com"}});
  CHECK(email.has_value());
  CHECK(email->as<bool>());
  try {
    (void)call("camelCase",std::string("\xC0\xAF"));
    CHECK(false);
  }catch(const std::invalid_argument&) {}
}
