// Native JSON-Lines bridge for exact pinned GutTS JSON.parse/stringify tests.
#include "gut/json.hpp"
#include <iostream>
#include <string>
int main(){
  std::string line;
  while(std::getline(std::cin,line)){
    try{std::cout<<gut::stringify_json(gut::parse_json(line))<<'\n';}
    catch(const std::exception& e){std::cerr<<"Native JSON failed: "<<e.what()<<'\n';return 1;}
  }
  return std::cin.eof()&&std::cout.good()?0:1;
}
