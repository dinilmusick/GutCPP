#include "gut/model_registry.hpp"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"model_registry_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
int main(){
 const auto& models=gut::gutts_model_definitions();
 CHECK(models.size()==8);
 std::set<std::string> ids;
 for(const auto& model:models){
   CHECK(ids.insert(model.id).second);
   CHECK(model.model_uri.has_value());
   CHECK(gut::gutts_model_by_id(model.id)==&model);
   CHECK(!model.capabilities.empty());
   CHECK(!model.runtime.empty());
   CHECK(model.size.value>0||model.size.min.has_value());
 }
 CHECK(models.at(0).id=="model.smollm2-135m-instruct");
 CHECK(models.at(1).id=="model.cactus-needle3");
 CHECK(models.at(7).id=="model.minilm-l6-v2");
 CHECK(models.at(1).size.min==8&&models.at(1).size.max==29);
 CHECK(models.at(6).size.value==150000000.0);
 CHECK(!gut::gutts_model_by_id("missing"));
}
