#pragma once
// Genuine Ettin ONNX transformer backbone + tokenizer + pinned float32 head.
// The injected model directory must contain real HF artifacts; never fake
// scores or substitute post-transformer head tests for model inference.
#include "gut/ettin_head.hpp"
#include "gut/json.hpp"
#include <libembedding/detail/onnx_session_impl.hpp>
#include <libembedding/detail/tokenizer_impl.hpp>
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
  if(value.empty()||value.size()>4096||value.find('\0')!=std::string_view::npos)
    throw std::invalid_argument(std::string(field)+" must be nonempty, NUL-free and <=4096 bytes");
  (void)utf8_scalars(value);
  return std::string(value);
}
inline std::size_t sequence_len(const std::vector<int64_t>& mask){
  std::size_t n=0;
  for(const auto val:mask){
    if(val==1)++n;
    else if(val!=0)throw std::runtime_error("Invalid native tokenizer attention mask");
  }
  return n;
}
inline std::vector<int64_t> pair_tokens(
    const lembed::detail::EncodingBatch& enc,std::vector<int64_t>& types) {
  if(enc.input_ids.size()!=2||enc.attention_mask.size()!=2||enc.token_type_ids.size()!=2)
    throw std::runtime_error("Ettin tokenizer failed to encode query/candidate");
  const auto nq=sequence_len(enc.attention_mask[0]),nd=sequence_len(enc.attention_mask[1]);
  if(nq<2||nd<2||nq+nd-1>512)
    throw std::invalid_argument("Ettin paired input exceeds 512 model tokens");
  const auto& q=enc.input_ids[0];
  const auto& d=enc.input_ids[1];
  if(q.size()<nq||d.size()<nd||q.front()!=d.front())
    throw std::runtime_error("Ettin tokenizer special-token layout unsupported");
  std::vector<int64_t> ids;
  ids.reserve(nq+nd-1);
  types.reserve(nq+nd-1);
  for(std::size_t i=0;i<nq;++i){ids.push_back(q[i]);types.push_back(0);}
  for(std::size_t i=1;i<nd;++i){ids.push_back(d[i]);types.push_back(1);}
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
    impl->tokenizer.load_from_file(root+"/tokenizer.json",512);
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
      auto batch=impl_->tokenizer.encode_batch({checked_query,text});
      std::vector<int64_t> types;
      const auto ids=ettin_native_detail::pair_tokens(batch,types);
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
    lembed::detail::TokenizerWrapper tokenizer;
    lembed::detail::OnnxSession onnx;
    ettin_head_weights head;
    int output_index=0;
    mutable std::mutex mu;
  };
  std::unique_ptr<Impl> impl_;
};
} // namespace gut
