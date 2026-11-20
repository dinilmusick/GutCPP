#include "gut/model_library.hpp"
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"model_library_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)

struct file_model {std::string bytes;};
struct filesystem_runtime final:gut::model_runtime<file_model>{
 std::atomic<int> loads{0};
 std::atomic<int> unloads{0};
 file_model load(const gut::model_metadata& metadata)override{
   if(metadata.id.empty())throw std::invalid_argument("missing model ID");
   std::ifstream input(GUTCPP_MODEL_LIBRARY_FIXTURE,std::ios::binary);
   if(!input)throw std::runtime_error("Real fixture missing");
   ++loads;
   return {std::string(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>())};
 }
 void unload(file_model& model)override{CHECK(!model.bytes.empty());model.bytes.clear();++unloads;}
};
int main(){
 const auto* metadata=gut::gutts_model_by_id("model.minilm-l6-v2");
 CHECK(metadata!=nullptr);
 auto runtime=std::make_shared<filesystem_runtime>();
 gut::model_library<file_model> lib(*metadata,runtime);
 CHECK(lib.metadata().id==metadata->id);
 CHECK(!lib.is_loaded()&&lib.active_users()==0);
 const auto bytes=lib.with_model([](file_model& model){return model.bytes.size();});
 CHECK(bytes>500);
 CHECK(!lib.is_loaded()&&lib.active_users()==0);
 CHECK(runtime->loads==1&&runtime->unloads==1);
 CHECK(lib.with_model([bytes](file_model& model){return model.bytes.size()==bytes;}));
 CHECK(runtime->loads==2&&runtime->unloads==2);
 bool rejected=false;
 try {gut::model_library<file_model> invalid(*metadata,nullptr);}
 catch(const std::invalid_argument&){rejected=true;}
 CHECK(rejected);
}
