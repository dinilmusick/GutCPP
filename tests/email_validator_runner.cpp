#include "gut/json.hpp"
#include "gut/strings.hpp"
#include <iostream>
#include <string>
int main(){
  std::string line;
  while(std::getline(std::cin,line)){
    try{
      const auto input=gut::parse_json(line).as<std::string>();
      std::cout<<(gut::is_email(input)?"true":"false")<<'\n';
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
  }
  return std::cin.eof()&&std::cout.good()?0:1;
}
