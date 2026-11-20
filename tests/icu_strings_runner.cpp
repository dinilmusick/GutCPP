// JSON-lines Unicode runner; no precomputed outcomes or emulated reference.
#include "gut/icu_strings.hpp"
#include "gut/json.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
int main(){
 std::string line;
 while(std::getline(std::cin,line)) {
  try {
   const auto body=gut::parse_json(line).as<gut::json_value::object>();
   const auto op=body.at("op").as<std::string>();
   const auto value=body.at("value").as<std::string>();
   std::string result;
   if(op=="capitalize")result=gut::icu_strings::capitalize(value);
   else if(op=="camelCase")result=gut::icu_strings::camel_case(value);
   else if(op=="kebabCase")result=gut::icu_strings::kebab_case(value);
   else if(op=="snakeCase")result=gut::icu_strings::snake_case(value);
   else if(op=="truncate") {
    auto len=body.at("maxLength").as<double>();
    auto suffix=body.at("suffix").as<std::string>();
    result=gut::icu_strings::truncate(value,static_cast<int64_t>(len),suffix);
   }else throw std::invalid_argument("unknown Unicode operation");
   std::cout<<gut::stringify_json(gut::json_value(gut::json_value::object{{"ok",true},{"value",result}}))<<'\n';
  }catch(const std::exception&){
   std::cout<<"{\"ok\":false}"<<'\n';
  }
 }
 return std::cin.eof()&&std::cout.good()?0:1;
}
