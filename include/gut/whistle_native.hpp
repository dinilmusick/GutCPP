#pragma once
// Optional real Cactus 4 speech-model API. Link libcactus from a pinned release.
// The runtime has PROCESS-GLOBAL model state, not an unloadable session handle.
#include "gut/json.hpp"
#include "gut/wav_pcm.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

extern "C" {
int cactus_load(const char*, std::uint64_t);
int cactus_transcribe(const float*,int,const char*,const char*,int,char*,int);
const char* cactus_last_error(void);
}

namespace gut {
namespace whistle_native_detail {
inline std::mutex engine_lock;
inline std::string active_model_path;
inline std::runtime_error error(std::string_view action) {
  const char* reason=cactus_last_error();
  return std::runtime_error(std::string("Cactus Whistle ")+std::string(action)+": "+
                            (reason&&*reason?reason:"native engine failed"));
}
inline std::string safe_input(std::string_view s,const char* name,bool may_be_empty=false) {
  if((!may_be_empty&&s.empty())||s.find('\0')!=std::string_view::npos)
    throw std::invalid_argument(std::string(name)+" has empty value or embedded NUL");
  (void)utf8_scalars(s);
  return std::string(s);
}
inline void validate_language(std::string_view v){
  if(v.empty())return;
  for(auto lang:{"en","de","fr","es","it","nl","pl"})if(v==lang)return;
  throw std::invalid_argument("unsupported Whistle language code");
}
}

// Cactus 4 has a single process-global model slot. WhistleNativeSession guards
// native calls within GutCPP but cannot isolate other users of libcactus. A
// destructor must NOT call cactus_reset: that would affect unrelated sessions.
class WhistleNativeSession {
public:
  explicit WhistleNativeSession(std::string_view model_file)
    : path_(whistle_native_detail::safe_input(model_file,"Whistle model path")) {
    std::lock_guard<std::mutex> lock(whistle_native_detail::engine_lock);
    activate_locked();
  }
  WhistleNativeSession(const WhistleNativeSession&)=delete;
  WhistleNativeSession& operator=(const WhistleNativeSession&)=delete;
  WhistleNativeSession(WhistleNativeSession&&)=default;
  WhistleNativeSession& operator=(WhistleNativeSession&&)=default;

  // Samples must be 16-kHz mono float32 in [-1, 1]. Returns actual Cactus 4
  // speech result with text, language, ttft_ms and decode_tps.
  json_value transcribe_detailed(const std::vector<float>& samples,
                                  std::string_view language={},bool word_timestamps=false) {
    whistle_native_detail::validate_language(language);
    if(samples.empty()||samples.size()>16000u*600u||
       samples.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))
      throw std::invalid_argument("Whistle needs 1..600 seconds of 16-kHz samples");
    for(float v:samples)if(!std::isfinite(v)||v < -1.f || v > 1.f)
      throw std::invalid_argument("nonfinite or out-of-range audio sample");
    const auto lang=whistle_native_detail::safe_input(language,"language",true);
    std::lock_guard<std::mutex> lock(whistle_native_detail::engine_lock);
    activate_locked();
    const std::size_t capacity=std::max<std::size_t>(1u<<18,
        ((samples.size()+15999u)/16000u)*4096u);
    if(capacity>static_cast<std::size_t>(std::numeric_limits<int>::max()))
      throw std::invalid_argument("Whistle response buffer too large");
    std::vector<char> output(capacity,0);
    const int result=cactus_transcribe(samples.data(),static_cast<int>(samples.size()),
      lang.empty()?nullptr:lang.c_str(),nullptr,word_timestamps?1:0,
      output.data(),static_cast<int>(capacity));
    if(result<0)throw whistle_native_detail::error("transcription failed");
    // Guard against native buffer overflow/unterminated output even after
    // nonnegative success; never parse out-of-bounds bytes as JSON.
    const auto end=std::find(output.begin(),output.end(),'\0');
    if(end==output.end())throw std::runtime_error("Whistle produced unterminated response");
    const auto parsed=parse_json(std::string(output.begin(),end));
    const auto& body=parsed.as<json_value::object>();
    if(!std::holds_alternative<std::string>(body.at("text").data)||
       !std::holds_alternative<std::string>(body.at("language").data))
      throw std::runtime_error("Whistle result has incorrect text/language types");
    return parsed;
  }

  std::string transcribe(const std::vector<float>& samples,
                         std::string_view language={}) {
    const auto obj=transcribe_detailed(samples,language).as<json_value::object>();
    return obj.at("text").as<std::string>();
  }

  // Input matches GutTS's byte-array audio shape for uncompressed WAVE data:
  // validate RIFF structure, decode PCM/IEEE float and resample to 16-kHz mono.
  // Compressed formats are rejected, never silently interpreted as PCM.
  json_value transcribe_detailed_wav(const std::vector<std::uint8_t>& wav_bytes,
                                      std::string_view language={},bool word_timestamps=false) {
    return transcribe_detailed(decode_wav_to_16khz(wav_bytes),language,word_timestamps);
  }
  std::string transcribe_wav(const std::vector<std::uint8_t>& wav_bytes,
                             std::string_view language={}) {
    return transcribe_detailed_wav(wav_bytes,language).as<json_value::object>()
      .at("text").as<std::string>();
  }
private:
  void activate_locked(){
    if(path_.empty())throw std::logic_error("Whistle session moved from");
    if(whistle_native_detail::active_model_path==path_)return;
    std::ifstream file(path_,std::ios::binary|std::ios::ate);
    if(!file)throw std::invalid_argument("Whistle model file unreadable");
    const auto size=file.tellg();
    if(size<=0||size>128*1024*1024)throw std::invalid_argument("Whistle model size invalid");
    std::vector<char> weight_bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if(!file.read(weight_bytes.data(),size))throw std::runtime_error("Whistle model read failed");
    if(cactus_load(weight_bytes.data(),static_cast<std::uint64_t>(weight_bytes.size()))<0) {
      whistle_native_detail::active_model_path.clear();
      throw whistle_native_detail::error("load failed");
    }
    whistle_native_detail::active_model_path=path_;
  }
  std::string path_;
};
} // namespace gut
