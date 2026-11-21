#pragma once
// Genuine Ettin ONNX transformer backbone + tokenizer + pinned float32 head.
// The injected model directory must contain real HF artifacts; never fake
// scores or substitute post-transformer head tests for model inference.
#include "gut/ettin_head.hpp"
#include "gut/json.hpp"
#include <libembedding/detail/onnx_session_impl.hpp>
#include "gut/ettin_tokenizer_hf.hpp"
#include <fstream>
#include <iterator>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
namespace ettin_native_detail {
inline std::string checked_text(std::string_view value,const char *field) {
  if(value.empty()||value.size()>65536||value.find('\0')!=std::string_view::npos)
    throw std::invalid_argument(std::string(field)+" must be nonempty, NUL-free and <=65536 bytes");
  (void)utf8_scalars(value);
  return std::string(value);
}
// The original lightweight libembedding BPE parser does NOT implement
// Hugging Face ByteLevel pre-tokenization or exact merge semantics; it
// returned 37 token IDs for a source fixture with 18. Load an authentic
// tokenizer.json through pinned Rust-backed HF tokenizers-cpp instead.
inline std::unique_ptr<HfTokenizer> load_tokenizer(const std::string& file) {
  std::ifstream input(file,std::ios::binary|std::ios::ate);
  if(!input)throw std::runtime_error("Ettin authentic tokenizer.json missing");
  const auto length=input.tellg();
  if(length<=0||length>64LL*1024LL*1024LL)
    throw std::runtime_error("Ettin tokenizer.json has unsafe size");
  std::string json(static_cast<std::size_t>(length),'\0');
  input.seekg(0);
  if(!input.read(json.data(),static_cast<std::streamsize>(json.size())))
    throw std::runtime_error("Could not load the complete Ettin tokenizer.json");
  auto result=std::make_unique<HfTokenizer>(json);
  if(!result||result->GetVocabSize()<10000)
    throw std::runtime_error("Genuine Ettin Hugging Face vocabulary invalid");
  return result;
}
inline std::vector<int64_t> pair_tokens(
    HfTokenizer& tokenizer,const std::string& query,
    const std::string& document,std::vector<int64_t>& types) {
  // Tokenizer::Encode calls HF Rust tokenizer with add_special_tokens=false.
  // The pinned model family expects an explicit BERT-style pair template:
  // [CLS] query [SEP] document [SEP]. Do not invoke independent full sentence
  // encodings with their own duplicated [CLS]/[SEP] handling.
  // The published Ettin family uses BERT's [CLS]/[SEP] special tokens
  // (IDs 50281/50282 at the pinned 17M revision), not RoBERTa's
  // <s>/</s>. Support either explicitly without inventing hardcoded IDs.
  auto first=tokenizer.TokenToId("[CLS]");
  auto sep=tokenizer.TokenToId("[SEP]");
  if(first<0)first=tokenizer.TokenToId("<s>");
  if(sep<0)sep=tokenizer.TokenToId("</s>");
  if(first<0||sep<0)
    throw std::runtime_error("Authentic Ettin tokenizer missing [CLS]/[SEP] or <s>/</s>");
  auto q=tokenizer.Encode(query);
  auto d=tokenizer.Encode(document);
  // At the pinned model revision, GutTS's Transformers.js tokenizer does
  // NOT cap this pair to 512 despite truncation=true: model_max_length is
  // not configured. Preserve exact token IDs including long sequences.
  // The native ONNX runtime enforces its supported execution context below.
  std::vector<int64_t> ids;
  ids.reserve(q.size()+d.size()+3);
  types.clear();types.reserve(ids.capacity());
  ids.push_back(first);types.push_back(0);
  for(const auto id:q) {ids.push_back(id);types.push_back(0);}
  ids.push_back(sep);types.push_back(0);
  for(const auto id:d) {ids.push_back(id);types.push_back(1);}
  ids.push_back(sep);types.push_back(1);
  return ids;
}
} // namespace ettin_native_detail

class EttinNativeSession final {
 public:
  explicit EttinNativeSession(std::string_view artifact_directory,int threads=2) {
    const auto root=ettin_native_detail::checked_text(artifact_directory,"Ettin model directory");
    if(threads<1||threads>32)
      throw std::invalid_argument("Ettin CPU thread count must be 1..32");
    auto impl=std::make_unique<Impl>();
    impl->tokenizer=ettin_native_detail::load_tokenizer(root+"/tokenizer.json");
    impl->head=load_ettin_head_weights(root); // real Safetensors head files
    impl->onnx.load_from_file((root+"/onnx/model.onnx").c_str(),threads,0);
    if(!impl->onnx.has_input("input_ids")||!impl->onnx.has_input("attention_mask"))
      throw std::runtime_error("Ettin ONNX backbone lacks token/attention inputs");
    const auto& outputs=impl->onnx.output_names();
    const auto it=std::find(outputs.begin(),outputs.end(),std::string("last_hidden_state"));
    if(it==outputs.end())
      throw std::runtime_error("Ettin ONNX model lacks genuine last_hidden_state");
    impl->output_index=static_cast<int>(it-outputs.begin());
    impl_=std::move(impl);
  }
  EttinNativeSession(const EttinNativeSession&)=delete;
  EttinNativeSession& operator=(const EttinNativeSession&)=delete;
  EttinNativeSession(EttinNativeSession&&)=default;
  EttinNativeSession& operator=(EttinNativeSession&&)=default;
  ~EttinNativeSession()=default;

  bool loaded()const noexcept{return static_cast<bool>(impl_);}
  void close()noexcept{impl_.reset();}
  // Return actual model scores aligned with caller's candidate order.
  std::vector<double> rerank(std::string_view query,
                             const std::vector<std::string>& candidates)const {
    const auto checked_query=ettin_native_detail::checked_text(query,"Ettin query");
    if(!impl_)throw std::runtime_error("Ettin ONNX session has closed or moved");
    if(candidates.size()>64)throw std::invalid_argument("Too many Ettin candidates");
    if(candidates.empty())return {};
    std::lock_guard<std::mutex> lock(impl_->mu);
    std::vector<double> scores;
    scores.reserve(candidates.size());
    for(const auto& candidate:candidates) {
      const auto text=ettin_native_detail::checked_text(candidate,"Ettin candidate");
      std::vector<int64_t> types;
      const auto ids=ettin_native_detail::pair_tokens(*impl_->tokenizer,checked_query,text,types);
      if(ids.size()>512)throw std::invalid_argument("Ettin native ONNX model context supports at most 512 tokens; tokenizer output is preserved without truncation");
      std::vector<int64_t> mask(ids.size(),1);
      const int64_t *type_ptr=impl_->onnx.has_input("token_type_ids")?types.data():nullptr;
      auto output=impl_->onnx.run(ids.data(),mask.data(),type_ptr,
                                 1,static_cast<int>(ids.size()),impl_->output_index);
      if(output.size()!=1||output[0].shape.size()!=3||output[0].shape[0]!=1||
         output[0].shape[1]!=static_cast<int64_t>(ids.size())||
         output[0].shape[2]<=0||output[0].shape[2]>4096)
        throw std::runtime_error("Ettin ONNX backbone has unsupported hidden-state shape");
      ettin_hidden_states hidden{
        1,ids.size(),static_cast<std::size_t>(output[0].shape[2]),
        std::move(output[0].data)
      };
      const auto score=compute_ettin_head_scores(hidden,impl_->head);
      if(score.size()!=1||!std::isfinite(score[0]))
        throw std::runtime_error("Real Ettin model produced an invalid score");
      scores.push_back(score[0]);
    }
    return scores;
  }
 private:
  struct Impl {
    std::unique_ptr<ettin_native_detail::HfTokenizer> tokenizer;
    lembed::detail::OnnxSession onnx;
    ettin_head_weights head;
    int output_index=0;
    mutable std::mutex mu;
  };
  std::unique_ptr<Impl> impl_;
};
} // namespace gut
