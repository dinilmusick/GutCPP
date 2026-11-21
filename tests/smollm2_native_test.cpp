#include "gut/smollm2_native.hpp"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

template<typename F>
static void must_throw(F&& f) {
  bool caught=false;
  try {f();} catch(const std::exception&) {caught=true;}
  if(!caught)throw std::runtime_error("Expected native negative-case failure");
}
int main(int argc,char **argv) {
  if(argc!=2) {
    std::cerr<<"Expected one genuine SmolLM2-135M-Instruct GGUF model path\n";
    return 2;
  }
  const std::string path=argv[1];
  must_throw([] {gut::SmolLM2NativeSession bad("/definitely/not/a/model.gguf");});
  must_throw([] {gut::SmolLM2NativeSession bad("");});
  gut::SmolLM2NativeSession model(path);
  assert(model.loaded());
  const std::string prompt="The capital of France is";
  assert(model.generate(prompt,0)==prompt);
  must_throw([&] {model.generate("",4);});
  must_throw([&] {model.generate("hello",1025);});
  must_throw([&] {model.generate(std::string("a\0b",3),4);});
  must_throw([&] {model.generate(std::string("\xC0\xAF",2),4);});
  const std::string actual=model.generate(prompt,16);
  if(actual.size()<=prompt.size() || actual.substr(0,prompt.size())!=prompt)
    throw std::runtime_error("Real native SmolLM2 generated no new text tokens");
  const std::string again=model.generate(prompt,16);
  if(actual!=again)throw std::runtime_error("Greedy real SmolLM2 generation not repeatable");
  gut::SmolLM2NativeSession moved(std::move(model));
  if(model.loaded()||!moved.loaded())
    throw std::runtime_error("SmolLM2 model move ownership incorrect");
  must_throw([&] {model.generate("test",4);});
  const std::string continuation=moved.generate("Hello, my name is",8);
  if(continuation.size()<=std::string("Hello, my name is").size())
    throw std::runtime_error("Moved native SmolLM2 session did not generate real continuation");
  moved.close();
  assert(!moved.loaded());
  must_throw([&] {moved.generate("test",4);});
  std::cout<<"SmolLM2 genuine native CPU model generation, greedy repeatability, ownership and negative tests passed\n";
  return 0;
}
