#include "gut/ettin_native.hpp"
#include <climits>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
static void out(const char *name,const std::vector<int64_t>& values){
  std::cout<<"\""<<name<<"\":[";
  for(std::size_t i=0;i<values.size();++i){
    if(i)std::cout<<",";
    std::cout<<values[i];
  }
  std::cout<<"]";
}
int main(int argc,char **argv){
  if(argc!=4)return 2;
  try{
    auto tokenizer=gut::ettin_native_detail::load_tokenizer(argv[1]);
    std::vector<int64_t> types;
    const auto ids=gut::ettin_native_detail::pair_tokens(
        *tokenizer,std::string(argv[2]),std::string(argv[3]),types);
    std::vector<int64_t> mask(ids.size(),1);
    std::cout<<"{";
    out("input_ids",ids);std::cout<<",";
    out("attention_mask",mask);std::cout<<",";
    out("token_type_ids",types);std::cout<<"}";
    return std::cout.good()?0:3;
  }catch(const std::exception& ex){
    std::cerr<<"Real native Ettin pair tokenizer error: "<<ex.what()<<"\n";
    return 4;
  }
}
