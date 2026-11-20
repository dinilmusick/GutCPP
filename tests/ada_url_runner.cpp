// Actual Ada URL parser JSON-lines driver for compiled GutTS differential CI.
#include "gut/ada_url.hpp"
#include "gut/json.hpp"
#include <exception>
#include <iostream>
#include <string>
int main(){
 std::string line;
 while(std::getline(std::cin,line)){
  try{
   const auto parsed=gut::parse_json(line);
   const auto& input=parsed.as<gut::json_value::object>();
   const auto& operation=input.at("op").as<std::string>();
   const auto& url=input.at("url").as<std::string>();
   gut::json_value::object result{{"ok",true}};
   if(operation=="isUrl")result.emplace("value",gut::is_url_whatwg(url));
   else if(operation=="getQueryParam"){
    const auto v=gut::get_query_param_whatwg(url,input.at("key").as<std::string>());
    if(v)result.emplace("value",*v);else result.emplace("value",nullptr);
   }
   else if(operation=="setQueryParam")result.emplace("value",gut::set_query_param_whatwg(url,input.at("key").as<std::string>(),input.at("value").as<std::string>()));
   else throw std::invalid_argument("unknown URL operation");
   std::cout<<gut::stringify_json(result)<<'\n';
  }catch(const std::exception&){std::cout<<R"({"ok":false})"<<'\n';}
 }
 return std::cin.eof()&&std::cout.good()?0:1;
}
