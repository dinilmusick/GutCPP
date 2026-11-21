#pragma once
// Exact Hugging Face Rust tokenizer.json backend via the minimal C ABI.
// Using tokenizers_cpp drags bundled SentencePiece/Protobuf into the ONNX
// process; on Linux that broke ONNX Protobuf model parsing. This thin C++17
// RAII adapter links only the pinned tokenizers-c Rust archive.
#include <tokenizers_c.h>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut::ettin_native_detail {
class HfTokenizer final {
 public:
  explicit HfTokenizer(const std::string& json) {
    if(json.empty()||json.size()>64U*1024U*1024U)
      throw std::invalid_argument("Invalid Hugging Face tokenizer.json size");
    handle_=tokenizers_new_from_str(json.data(),json.size());
    if(!handle_)throw std::runtime_error("Hugging Face tokenizer rejected tokenizer.json");
  }
  HfTokenizer(const HfTokenizer&)=delete;
  HfTokenizer& operator=(const HfTokenizer&)=delete;
  HfTokenizer(HfTokenizer&& other)noexcept :handle_(std::exchange(other.handle_,nullptr)){}
  HfTokenizer& operator=(HfTokenizer&& other)noexcept {
    if(this!=&other){close();handle_=std::exchange(other.handle_,nullptr);}
    return *this;
  }
  ~HfTokenizer(){close();}
  void close()noexcept{if(handle_){tokenizers_free(handle_);handle_=nullptr;}}
  std::size_t GetVocabSize()const {
    require();size_t size=0;tokenizers_get_vocab_size(handle_,&size);return size;
  }
  int TokenToId(std::string_view token)const {
    require();int32_t id=-1;
    tokenizers_token_to_id(handle_,token.data(),token.size(),&id);
    return id;
  }
  std::vector<int32_t> Encode(const std::string& text)const {
    require();
    TokenizerEncodeResult result{};
    tokenizers_encode(handle_,text.data(),text.size(),0,&result);
    struct ResultGuard {
      TokenizerEncodeResult& value;
      ~ResultGuard(){tokenizers_free_encode_results(&value,1);}
    } guard{result};
    if(!result.token_ids||result.len>131072)
      throw std::runtime_error("Hugging Face tokenizer returned an invalid encoding");
    return std::vector<int32_t>(result.token_ids,result.token_ids+result.len);
  }
 private:
  void require()const {
    if(!handle_)throw std::runtime_error("Hugging Face tokenizer is closed");
  }
  TokenizerHandle handle_=nullptr;
};
} // namespace gut::ettin_native_detail
