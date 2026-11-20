#include "gut/ettin_head.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>
#define CHECK(x) do{if(!(x)){std::cerr<<"ettin_head_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
using namespace gut;
static std::string make_safetensors(std::string name,const std::vector<float>& values){
  std::string header="{\""+name+"\":{\"dtype\":\"F32\",\"shape\":["+std::to_string(values.size())+"]"+
    ",\"data_offsets\":[0,"+std::to_string(values.size()*4)+"]}}";
  std::string bytes(8,'\0');
  for(unsigned i=0;i<8;++i)bytes[i]=static_cast<char>((header.size()>>(8*i))&255);
  bytes+=header;
  for(float f:values){std::uint32_t bits;std::memcpy(&bits,&f,4);for(unsigned i=0;i<4;++i)bytes+=static_cast<char>((bits>>(8*i))&255);}
  return bytes;
}
static bool rejects(std::string_view data){try{(void)parse_safetensors_f32(data,"weights");return false;}catch(const std::exception&){return true;}}
int main(){
  auto payload=make_safetensors("weights",{1.0f,-2.0f,0.25f});
  const auto parsed=parse_safetensors_f32(payload,"weights");
  CHECK(parsed.size()==3&&parsed[0]==1.0f&&parsed[1]==-2.0f&&parsed[2]==.25f);
  CHECK(rejects(payload.substr(0,3)));
  CHECK(rejects(payload.substr(0,payload.size()-1)));
  CHECK(rejects(""));
  CHECK(rejects(make_safetensors("other",{1.0f})));
  // Real filesystem-backed Safetensors end-to-end loading, not mocked bytes.
  namespace fs=std::filesystem;
  const auto directory=fs::temp_directory_path() /
    ("gutcpp-ettin-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  fs::create_directories(directory/"2_Dense");
  fs::create_directories(directory/"3_LayerNorm");
  fs::create_directories(directory/"4_Dense");
  const auto write=[&](const fs::path& path,const std::string& bytes){
    std::ofstream out(path,std::ios::binary);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
    if(!out)throw std::runtime_error("Failed to write Safetensors regression fixture");
  };
  write(directory/"2_Dense"/"model.safetensors",make_safetensors("linear.weight",{1,0,0,1}));
  // LayerNorm requires two F32 tensors in one file, so a valid combined fixture
  // is added by the helper below in this exact test.
  const auto normHeader=std::string("{\"norm.weight\":{\"dtype\":\"F32\",\"data_offsets\":[0,8]},")+
    "\"norm.bias\":{\"dtype\":\"F32\",\"data_offsets\":[8,16]}}";
  std::string normBytes(8,'\0');
  for(unsigned i=0;i<8;++i)normBytes[i]=static_cast<char>((normHeader.size()>>(8*i))&255);
  normBytes+=normHeader;
  for(float f:{1.0f,1.0f,0.0f,0.0f}){
    std::uint32_t bits;std::memcpy(&bits,&f,4);
    for(unsigned i=0;i<4;++i)normBytes+=static_cast<char>((bits>>(8*i))&255);
  }
  write(directory/"3_LayerNorm"/"model.safetensors",normBytes);
  const auto finalHeader=std::string("{\"linear.weight\":{\"dtype\":\"F32\",\"data_offsets\":[0,8]},")+
    "\"linear.bias\":{\"dtype\":\"F32\",\"data_offsets\":[8,12]}}";
  std::string finalBytes(8,'\0');
  for(unsigned i=0;i<8;++i)finalBytes[i]=static_cast<char>((finalHeader.size()>>(8*i))&255);
  finalBytes+=finalHeader;
  for(float f:{.25f,-.5f,.1f}){
    std::uint32_t bits;std::memcpy(&bits,&f,4);
    for(unsigned i=0;i<4;++i)finalBytes+=static_cast<char>((bits>>(8*i))&255);
  }
  write(directory/"4_Dense"/"model.safetensors",finalBytes);
  const auto loaded=load_ettin_head_weights(directory.string());
  CHECK(loaded.w1.size()==4&&loaded.ln_weight.size()==2&&loaded.ln_bias.size()==2);
  CHECK(loaded.w2.size()==2&&loaded.b2.size()==1);
  const auto testScores=compute_ettin_head_scores({1,1,2,{.1f,.2f}},loaded);
  CHECK(testScores.size()==1&&testScores[0]>0&&testScores[0]<1);
  fs::remove_all(directory);
  ettin_hidden_states hidden{2,2,3,{.1f,.2f,.3f,0,0,0,-.3f,.7f,.4f,0,0,0}};
  ettin_head_weights head{{.8f,.2f,-.1f,-.3f,.7f,.2f,.5f,-.2f,.1f},{1,1,1},{0,0,0},{.3f,-.2f,.8f},{.15f}};
  auto scores=compute_ettin_head_scores(hidden,head);
  CHECK(scores.size()==2&&scores[0]>0&&scores[0]<1&&scores[1]>0&&scores[1]<1);
  CHECK(scores[0]!=scores[1]);
  hidden.data.pop_back();
  try{(void)compute_ettin_head_scores(hidden,head);CHECK(false);}catch(const std::invalid_argument&){}
  hidden.data.push_back(0);
  head.w1.pop_back();
  try{(void)compute_ettin_head_scores(hidden,head);CHECK(false);}catch(const std::invalid_argument&){}
  head.w1.push_back(.1f);
  head.b2[0]=std::numeric_limits<float>::infinity();
  try{(void)compute_ettin_head_scores(hidden,head);CHECK(false);}catch(const std::invalid_argument&){}
}
