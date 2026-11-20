#include "gut/needle3_native.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#define CHECK(x) do{if(!(x)){std::cerr<<"Needle3 test line "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(false)
int main(int argc,char**argv){
  if(argc!=2){std::cerr<<"Usage: needle3_native_test <real needle3.cact>\n";return 2;}
  bool rejected=false;
  try{gut::Needle3Session invalid("/no-such-model-needle3.cact");}
  catch(const std::runtime_error&){rejected=true;}
  CHECK(rejected);
  rejected=false;
  try{gut::Needle3Session invalid(std::string("file\0garbage",12));}
  catch(const std::invalid_argument&){rejected=true;}
  CHECK(rejected);
  gut::Needle3Session live(argv[1]);
  CHECK(live.layers()>=6);
  gut::Needle3Session session(std::move(live));
  CHECK(live.layers()==0);
  CHECK(session.layers()>=6);
  const auto tools=gut::parse_json(R"([{"name":"get_weather","description":"Look up current weather in a city","parameters":{"type":"object","properties":{"city":{"type":"string"}},"required":["city"]}}])").as<gut::json_value::array>();
  const auto calls=session.tool_calls("What's the weather in Paris?",tools);
  CHECK(calls.has_value());
  const auto& arr=calls->as<gut::json_value::array>();
  CHECK(!arr.empty());
  const auto& first=arr.front().as<gut::json_value::object>();
  CHECK(first.at("name").as<std::string>()=="get_weather");
  CHECK(std::holds_alternative<gut::json_value::object>(first.at("arguments").data));
  std::cout<<"REAL Needle3 model="<<argv[1]<<" layers="<<session.layers()
           <<" output="<<gut::stringify_json(*calls)<<'\n';
  return 0;
}
