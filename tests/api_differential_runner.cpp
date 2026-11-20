// JSON-Lines bridge for executable pinned GutTS/C++ differential verification.
// No test data or expected outputs are embedded in this runner.
#include "gut/api.hpp"
#include "gut/json.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
int main(){
 std::string line;
 while(std::getline(std::cin,line)){
   try {
     const auto request=gut::parse_json(line);
     const auto& envelope=request.as<gut::json_value::object>();
     const auto id=envelope.at("apiId").as<std::string>();
     const auto& input=envelope.at("input").as<gut::json_value::object>();
     const auto value=gut::execute_gutts_api(id,input);
     gut::json_value::object response{{"ok",true},{"defined",value.has_value()}};
     if(value)response.emplace("value",*value);
     std::cout<<gut::stringify_json(gut::json_value(std::move(response)))<<'\n';
   }catch(const std::exception& ex){
     gut::json_value::object response{{"ok",false},{"error",std::string(ex.what())}};
     std::cout<<gut::stringify_json(gut::json_value(std::move(response)))<<'\n';
   }
 }
 if(!std::cin.eof())return 1;
 return std::cout.good()?0:1;
}
