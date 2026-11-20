#include "gut/whistle_native.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"Whistle test line "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(false)
int main(int argc,char**argv){
  if(argc!=2){std::cerr<<"Usage: whistle_native_test <real whistle.cact>\n";return 2;}
  bool rejected=false;
  try{gut::WhistleNativeSession bad("/not-a-real-whistle-model.cact");}
  catch(const std::invalid_argument&){rejected=true;}
  CHECK(rejected);
  gut::WhistleNativeSession model(argv[1]);
  try{(void)model.transcribe({std::numeric_limits<float>::quiet_NaN()});CHECK(false);}
  catch(const std::invalid_argument&){}
  try{(void)model.transcribe({1.1f});CHECK(false);}
  catch(const std::invalid_argument&){}
  try{(void)model.transcribe(std::vector<float>(16000,0.f),"xx");CHECK(false);}
  catch(const std::invalid_argument&){}
  // Exercise actual byte-oriented Whistle audio through the native decoder.
  // 16-kHz mono PCM16 is common for the GutTS Uint8Array input contract.
  auto pcm_wav=[](const std::vector<float>& input){
    std::vector<std::uint8_t> result;
    auto u16=[&](unsigned v){result.push_back(static_cast<std::uint8_t>(v));result.push_back(static_cast<std::uint8_t>(v>>8));};
    auto u32=[&](std::uint32_t v){u16(v&65535u);u16(v>>16);};
    auto tag=[&](const char* v){for(int i=0;i<4;++i)result.push_back(static_cast<std::uint8_t>(v[i]));};
    tag("RIFF");u32(static_cast<std::uint32_t>(36+input.size()*2));tag("WAVE");
    tag("fmt ");u32(16);u16(1);u16(1);u32(16000);u32(32000);u16(2);u16(16);
    tag("data");u32(static_cast<std::uint32_t>(input.size()*2));
    for(float value:input){
      const auto v=static_cast<std::int16_t>(std::lround(std::clamp(value,-1.0f,0.999969f)*32768.0f));
      u16(static_cast<std::uint16_t>(v));
    }
    return result;
  };
  try{(void)model.transcribe_wav({0,1,2,3});CHECK(false);}
  catch(const std::invalid_argument&){}
  const auto silence=std::vector<float>(16000,0.f);
  const auto silent_result=model.transcribe_detailed(silence);
  const auto& silent=silent_result.as<gut::json_value::object>();
  CHECK(silent.at("text").as<std::string>().empty());
  CHECK(silent.at("language").as<std::string>().empty());
  CHECK(silent.find("ttft_ms")!=silent.end());
  CHECK(silent.find("decode_tps")!=silent.end());
  CHECK(model.transcribe_wav(pcm_wav(silence)).empty());
  // A real three-second non-silent input exercises encoder+decoder rather
  // than accepting only a short-circuited silence branch.
  std::vector<float> samples(3*16000);
  for(std::size_t i=0;i<samples.size();++i){
    const auto phase=6.283185307179586*220.0*static_cast<double>(i)/16000.0;
    const auto envelope=0.2+0.8*std::fabs(std::sin(3.141592653589793*3.0*static_cast<double>(i)/16000.0));
    samples[i]=static_cast<float>(0.3*std::sin(phase)*envelope);
  }
  const auto detailed=model.transcribe_detailed(samples,"en",true);
  const auto& obj=detailed.as<gut::json_value::object>();
  CHECK(obj.at("language").as<std::string>()=="en");
  CHECK(std::holds_alternative<std::string>(obj.at("text").data));
  CHECK(std::holds_alternative<gut::json_value::array>(obj.at("words").data));
  const auto wav_detailed=model.transcribe_detailed_wav(pcm_wav(samples),"en",true);
  const auto& wav_obj=wav_detailed.as<gut::json_value::object>();
  CHECK(wav_obj.at("language").as<std::string>()=="en");
  CHECK(std::holds_alternative<std::string>(wav_obj.at("text").data));
  CHECK(std::holds_alternative<gut::json_value::array>(wav_obj.at("words").data));
  std::cout<<"REAL Whistle engine: "<<gut::stringify_json(detailed)<<'\n';
  return 0;
}
