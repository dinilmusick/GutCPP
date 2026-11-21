#pragma once
// Real MiniLM-L6-v2 ONNX transformer inference through pinned libembedding C API.
// Does NOT synthesize embeddings or substitute a mock inference backend.
// Requires explicit gutcpp::minilm_native CMake target (not the portable core).
#include "gut/minilm_math.hpp"
#include "gut/portable.hpp"
#include <libembedding/text_embedding.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
namespace minilm_detail {
inline std::vector<unsigned char> read_bytes(const std::string& path,std::size_t max_size){
  if(path.empty()||path.find('\0')!=std::string::npos)
    throw std::invalid_argument("MiniLM model/tokenizer path must be nonempty without NUL");
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  if(!file)throw std::invalid_argument("MiniLM model/tokenizer file cannot be opened");
  const auto size=file.tellg();
  if(size<=0||static_cast<std::uint64_t>(size)>max_size)
    throw std::invalid_argument("MiniLM model/tokenizer file size invalid");
  std::vector<unsigned char> result(static_cast<std::size_t>(size));
  file.seekg(0,std::ios::beg);
  if(!file.read(reinterpret_cast<char*>(result.data()),size))
    throw std::invalid_argument("MiniLM model/tokenizer file truncated");
  return result;
}
inline void validate_text(std::string_view input){
  if(input.size()>65536||input.find('\0')!=std::string_view::npos)
    throw std::invalid_argument("MiniLM text length or NUL invalid");
  (void)utf8_scalars(input);
}
inline std::string native_error(const char* operation){
  const char* detail=lembed_last_error();
  return std::string(operation)+": "+(detail&&*detail?detail:"ONNX/tokenizer error");
}
struct result_guard{
  lembed_embeddings_t value{};
  result_guard()=default;
  result_guard(const result_guard&)=delete;
  result_guard& operator=(const result_guard&)=delete;
  ~result_guard(){lembed_embeddings_free(&value);}
};
}
class MiniLMNativeSession final {
public:
  // Both artifacts must come from the SAME immutable HuggingFace commit:
  // sentence-transformers/all-MiniLM-L6-v2@1110a243fdf4706b3f48f1d95db1a4f5529b4d41
  MiniLMNativeSession(const std::string& onnx_model_file,
                      const std::string& tokenizer_json_file,int threads=2)
    : model_bytes_(minilm_detail::read_bytes(onnx_model_file,192u*1024u*1024u)),
      tokenizer_bytes_(minilm_detail::read_bytes(tokenizer_json_file,16u*1024u*1024u)) {
    if(threads<1||threads>64)throw std::invalid_argument("MiniLM thread count invalid");
    lembed_user_defined_model_t spec{};
    spec.onnx_data=model_bytes_.data();spec.onnx_data_size=model_bytes_.size();
    spec.tokenizer_json=tokenizer_bytes_.data();spec.tokenizer_json_size=tokenizer_bytes_.size();
    spec.pooling=LEMBED_POOLING_MEAN;
    spec.dim=384;
    spec.max_length=512;
    const auto status=lembed_text_embedding_create_custom(&spec,LEMBED_PROVIDER_CPU,threads,&ctx_);
    if(status!=LEMBED_OK||!ctx_){
      const auto message=minilm_detail::native_error("MiniLM failed to load real ONNX model");
      if(ctx_)lembed_text_embedding_free(ctx_);
      ctx_=nullptr;
      throw std::runtime_error(message);
    }
    if(lembed_text_embedding_dim(ctx_)!=384){
      lembed_text_embedding_free(ctx_);ctx_=nullptr;
      throw std::runtime_error("MiniLM model returned non-384 embedding width");
    }
  }
  ~MiniLMNativeSession(){close();}
  MiniLMNativeSession(const MiniLMNativeSession&)=delete;
  MiniLMNativeSession& operator=(const MiniLMNativeSession&)=delete;
  MiniLMNativeSession(MiniLMNativeSession&& other) noexcept
    :ctx_(std::exchange(other.ctx_,nullptr)),model_bytes_(std::move(other.model_bytes_)),
     tokenizer_bytes_(std::move(other.tokenizer_bytes_)){}
  MiniLMNativeSession& operator=(MiniLMNativeSession&& other) noexcept{
    if(this!=&other){close();ctx_=std::exchange(other.ctx_,nullptr);
      model_bytes_=std::move(other.model_bytes_);
      tokenizer_bytes_=std::move(other.tokenizer_bytes_);}
    return *this;
  }
  void close() noexcept{
    if(ctx_){lembed_text_embedding_free(ctx_);ctx_=nullptr;}
  }
  bool loaded() const noexcept{return ctx_!=nullptr;}
  std::vector<float> embed_batch(const std::vector<std::string>& texts){
    if(!ctx_)throw std::runtime_error("MiniLM model session closed");
    if(texts.empty()||texts.size()>64)throw std::invalid_argument("MiniLM batch size invalid");
    std::vector<const char*> refs;refs.reserve(texts.size());
    for(const auto& s:texts){minilm_detail::validate_text(s);refs.push_back(s.c_str());}
    minilm_detail::result_guard result;
    const auto status=lembed_text_embedding_embed(ctx_,refs.data(),static_cast<int>(refs.size()),
                                                  static_cast<int>(refs.size()),&result.value);
    if(status!=LEMBED_OK)throw std::runtime_error(minilm_detail::native_error("MiniLM inference failed"));
    if(result.value.num_embeddings!=static_cast<int>(texts.size())||result.value.dim!=384||!result.value.data)
      throw std::runtime_error("MiniLM model returned invalid embedding tensor");
    const std::size_t elements=texts.size()*384u;
    const std::vector<float> output(result.value.data,result.value.data+elements);
    if(std::any_of(output.begin(),output.end(),[](float x){return !std::isfinite(x);}))
      throw std::runtime_error("MiniLM model returned nonfinite embedding");
    return output;
  }
  std::vector<float> embed(const std::string& text){return embed_batch({text});}
  double similarity(const std::string& left,const std::string& right){
    return minilm_similarity_from_pooled_embeddings(embed_batch({left,right}));
  }
private:
  lembed_text_embedding_t* ctx_=nullptr;
  // Preserve ONNX buffer for entire native session lifetime, even if the ONNX
  // backend defers access to the original bytes after session construction.
  std::vector<unsigned char> model_bytes_,tokenizer_bytes_;
};
} // namespace gut
