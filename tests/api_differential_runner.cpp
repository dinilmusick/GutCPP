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
     // Emit the full published contract registry for pinned TS/C++ comparison.
     if(envelope.find("registry")!=envelope.end()) {
       gut::json_value::array registry;
       for(const auto& api:gut::gutts_api_definitions()) {
         gut::json_value::array parameters;
         for(const auto& param:api.parameters)parameters.emplace_back(gut::json_value::object{
           {"name",param.name},{"required",param.required},{"jsonType",param.json_type}});
         registry.emplace_back(gut::json_value::object{
           {"id",api.id},{"name",api.name},{"category",api.category},
           {"parameters",std::move(parameters)},{"outputType",api.output_type},
           {"deterministic",api.deterministic},{"pure",api.pure},{"sideEffects",api.side_effects}});
       }
       std::cout<<gut::stringify_json(gut::json_value(gut::json_value::object{
         {"ok",true},{"registry",std::move(registry)}}))<<'\n';
       continue;
     }
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
