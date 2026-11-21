#include "gut/ettin_native.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
template<class F>static void reject(F&& f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}if(!threw)throw std::runtime_error("Expected native Ettin failure");}
int main(int argc,char **argv){
  if(argc!=2){std::cerr<<"Usage: real_ettin_test path/to/pinned-real-model\n";return 2;}
  const std::string directory=argv[1];
  reject([]{gut::EttinNativeSession bad("/definitely/missing");});
  reject([]{gut::EttinNativeSession bad("");});
  gut::EttinNativeSession model(directory,2);
  assert(model.loaded());
  const std::string query="Which planet is known as the Red Planet?";
  const std::vector<std::string> candidates={
    "Mars is known for its red surface and is called the Red Planet.",
    "Venus is Earth's twin because of its similar size.",
    "Jupiter has a giant red spot in its atmosphere."
  };
  auto scores=model.rerank(query,candidates);
  if(scores.size()!=3)throw std::runtime_error("Ettin ONNX did not score all candidates");
  for(double v:scores)if(!std::isfinite(v)||v<=0||v>=1)throw std::runtime_error("Ettin real score invalid");
  if(scores[0]==scores[1]&&scores[1]==scores[2])
    throw std::runtime_error("Different real Ettin candidate pairs produced identical scores");
  const auto again=model.rerank(query,candidates);
  if(again!=scores)throw std::runtime_error("Ettin deterministic CPU model changed repeat scoring");
  if(!model.rerank(query,{}).empty())throw std::runtime_error("Empty Ettin ranking must be empty");
  reject([&]{model.rerank("",{"Mars"});});
  reject([&]{model.rerank(query,{""});});
  reject([&]{model.rerank(std::string("\xC0\xAF",2),{"Mars"});});
  reject([&]{model.rerank(query,std::vector<std::string>(65,"candidate"));});
  reject([&]{model.rerank(query,{std::string(70000,'x')});});
  // Authentic tokenizer may generate >512 IDs because pinned
  // Transformers.js has no configured tokenizer-side max_length.
  // The native ONNX provider must reject unsupported model context
  // instead of silently changing the source tokenizer's IDs.
  std::string onnx_overlong;
  for(int i=0;i<110;++i)onnx_overlong+="Mars is the famous Red Planet. ";
  reject([&]{model.rerank(query,{onnx_overlong});});
  gut::EttinNativeSession moved(std::move(model));
  if(!moved.loaded()||model.loaded())throw std::runtime_error("Ettin native move ownership failed");
  reject([&]{model.rerank(query,{"Mars"});});
  if(moved.rerank("What is the red planet?",{"Mars"}).size()!=1)
    throw std::runtime_error("Moved Ettin session failed to execute ONNX model");
  moved.close();
  if(moved.loaded())throw std::runtime_error("Ettin model close failed");
  reject([&]{moved.rerank(query,{"Mars"});});
  std::cout<<"Genuine Ettin ONNX tokenizer/backbone + real Safetensors head, repetition, ownership and negative tests passed\n";
}
