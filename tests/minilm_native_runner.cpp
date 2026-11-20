// Actual native MiniLM ONNX embedding runner; never computes expected values.
#include "gut/minilm_native.hpp"
#include "gut/json.hpp"
#include <exception>
#include <iostream>
#include <string>
int main(int argc,char** argv){
  if(argc!=3){std::cerr<<"usage: minilm_native_runner MODEL.onnx tokenizer.json\n";return 2;}
  try{
    gut::MiniLMNativeSession model(argv[1],argv[2]);
    std::string line;
    while(std::getline(std::cin,line)){
      try{
        const auto in=gut::parse_json(line).as<gut::json_value::object>();
        const auto left=in.at("left").as<std::string>();
        const auto right=in.at("right").as<std::string>();
        const auto similarity=model.similarity(left,right);
        std::cout<<gut::stringify_json(gut::json_value(similarity))<<'\n';
      }catch(const std::exception& ex){
        std::cerr<<"Native MiniLM input rejected: "<<ex.what()<<'\n';return 1;
      }
    }
    return std::cin.eof()&&std::cout.good()?0:1;
  }catch(const std::exception& ex){std::cerr<<"Native MiniLM unavailable: "<<ex.what()<<'\n';return 1;}
}
