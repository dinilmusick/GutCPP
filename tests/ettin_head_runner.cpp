#include "gut/ettin_head.hpp"
#include <exception>
#include <iostream>
#include <string>
#include <vector>
using namespace gut;
static std::vector<float> floats(const json_value& v){
  std::vector<float> out;
  const auto& a=v.as<json_value::array>();out.reserve(a.size());
  for(const auto& n:a)out.push_back(static_cast<float>(n.as<double>()));
  return out;
}
static std::size_t nonnegative_size(const json_value& v) {
  const double n=v.as<double>();
  if(!std::isfinite(n)||n<0||std::floor(n)!=n||n>100000)
    throw std::invalid_argument("Invalid dimension");
  return static_cast<std::size_t>(n);
}
int main(){
  std::string line;
  while(std::getline(std::cin,line)){
    try{
      const auto request=parse_json(line);
      const auto& obj=request.as<json_value::object>();
      const auto& hidden=obj.at("hidden").as<json_value::object>();
      const auto& dims=hidden.at("dims").as<json_value::array>();
      if(dims.size()!=3)throw std::invalid_argument("Three hidden tensor dimensions required");
      const ettin_hidden_states states{nonnegative_size(dims[0]),nonnegative_size(dims[1]),nonnegative_size(dims[2]),floats(hidden.at("data"))};
      const auto& h=obj.at("head").as<json_value::object>();
      const ettin_head_weights weights{floats(h.at("w1")),floats(h.at("lnWeight")),floats(h.at("lnBias")),floats(h.at("w2")),floats(h.at("b2"))};
      json_value::array result;
      for(double score:compute_ettin_head_scores(states,weights))result.emplace_back(score);
      std::cout<<stringify_json(json_value(std::move(result)))<<'\n';
    }catch(const std::exception& ex){std::cerr<<"Failed Ettin differential input: "<<ex.what()<<'\n';return 1;}
  }
  return std::cin.eof()&&std::cout.good()?0:1;
}
