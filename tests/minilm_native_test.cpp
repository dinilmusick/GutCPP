#include "gut/minilm_native.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#define CHECK(x) do{if(!(x)){std::cerr<<"MiniLM native test "<<__LINE__<<" failed: "<<#x<<'\n';return 1;}}while(false)
int main(int argc,char** argv){
  if(argc!=3) {std::cerr<<"usage: minilm_native_test MODEL.onnx tokenizer.json\n";return 2;}
  bool rejected=false;
  try{gut::MiniLMNativeSession bad("/does/not/exist.onnx",argv[2]);}
  catch(const std::invalid_argument&){rejected=true;}
  CHECK(rejected);
  rejected=false;try{gut::MiniLMNativeSession bad(argv[1],argv[2],0);}
  catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
  gut::MiniLMNativeSession model(argv[1],argv[2]);
  CHECK(model.loaded());
  const auto embedded=model.embed("A cat is sitting on the sofa");
  CHECK(embedded.size()==384);
  double norm=0.0;for(float f:embedded){CHECK(std::isfinite(f));norm+=double(f)*f;}
  CHECK(std::fabs(norm-1.0)<.005);
  const auto same=model.similarity("A cat is sitting on the sofa","A cat is sitting on the sofa");
  CHECK(same>.9999&&same<=1.0001);
  const auto related=model.similarity("A cat is sitting on the sofa","A kitten is resting on a couch");
  const auto unrelated=model.similarity("A cat is sitting on the sofa","Quantum mechanics in black holes");
  CHECK(std::isfinite(related)&&std::isfinite(unrelated));
  CHECK(related>unrelated);
  rejected=false;try{(void)model.embed(std::string("x\0y",3));}
  catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
  rejected=false;try{(void)model.embed(std::string("\xC0\xAF",2));}
  catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
  gut::MiniLMNativeSession moved(std::move(model));
  CHECK(!model.loaded()&&moved.loaded());
  CHECK(moved.embed("test").size()==384);
  moved.close();CHECK(!moved.loaded());
  rejected=false;try{(void)moved.embed("closed");}
  catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
  std::cout<<"Real MiniLM ONNX model inference, embedding 384 and semantic ranking passed\n";
  return 0;
}
