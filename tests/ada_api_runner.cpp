// JSON-Lines real GutTS API registry URL dispatcher using pinned Ada WHATWG.
#include "gut/api.hpp"
#include <exception>
#include <iostream>
#include <string>
int main(){
  std::string line;
  while(std::getline(std::cin,line)){
    try{
      const auto request=gut::parse_json(line);
      const auto& body=request.as<gut::json_value::object>();
      const auto& op=body.at("op").as<std::string>();
      const auto& url=body.at("url").as<std::string>();
      gut::json_value::object input;
      if(op=="isUrl")input.emplace("value",url);
      else{
        input.emplace("url",url);
        input.emplace("key",body.at("key"));
        if(op=="setQueryParam")input.emplace("value",body.at("value"));
      }
      const auto result=gut::execute_gutts_api("gutts.urls."+op,input);
      if(!result)throw std::logic_error("URL API returned undefined");
      std::cout<<"{\"ok\":true,\"value\":"<<gut::stringify_json(*result)<<"}\n";
    }catch(const std::exception&){
      std::cout<<"{\"ok\":false}\n";
    }
  }
  return std::cin.eof()&&std::cout.good()?0:1;
}
