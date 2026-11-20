// Strict JSON-Lines runner for real pinned GutTS native MiniLM cosine parity.
// It does not generate embeddings; only compares numeric model postprocessing.
#include "gut/minilm_math.hpp"
#include "gut/json.hpp"
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
namespace {
std::vector<float> array_values(const gut::json_value& value) {
  const auto& values=value.as<gut::json_value::array>();
  std::vector<float> result;result.reserve(values.size());
  for(const auto& item:values)result.push_back(static_cast<float>(item.as<double>()));
  return result;
}
}
int main(){
 std::string line;
 while(std::getline(std::cin,line)){
   try {
     const auto req=gut::parse_json(line).as<gut::json_value::object>();
     const auto& kind=req.at("kind").as<std::string>();
     double value=0;
     if(kind=="cosine")value=gut::minilm_cosine(array_values(req.at("left")),array_values(req.at("right")));
     else if(kind=="pooled")value=gut::minilm_similarity_from_pooled_embeddings(array_values(req.at("pooled")));
     else throw std::invalid_argument("Unknown MiniLM numeric operation");
     gut::json_value::object response{{"ok",true}};
     if(std::isnan(value))response.emplace("kind","nan");
     else if(std::isinf(value))response.emplace("kind",value>0?"pos_inf":"neg_inf");
     else {response.emplace("kind","finite");response.emplace("value",value);}
     std::cout<<gut::stringify_json(response)<<'\n';
   }catch(const std::exception&){
     std::cout<<R"({"ok":false,"kind":"error"})"<<'\n';
   }
 }
 return std::cin.eof()&&std::cout.good()?0:1;
}
