// Standalone real installed consumer: no repository include directories.
#include <gut/api.hpp>
#include <iostream>
#include <string>
int main() {
#ifndef GUTCPP_WITH_ICU_STRINGS
#error "Installing and linking gutcpp::icu must enable Unicode API routing"
#endif
  const gut::json_value::object args{{"value","café Crème"}};
  const auto value=gut::execute_gutts_api("gutts.strings.camelCase",args);
  if(!value||value->as<std::string>()!="cafeCreme") {
    std::cerr<<"installed Unicode consumer returned wrong API output\n";
    return 1;
  }
  return 0;
}
