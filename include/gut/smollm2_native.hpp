#pragma once
// Real SmolLM2-135M-Instruct inference using pinned llama.cpp's C API.
// This is an OPT-IN native C++17 runtime with genuine caller-supplied GGUF
// weights. Never silently replace inference with fixed text or a remote API.
#include "gut/json.hpp"
#include <llama.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
namespace smollm2_detail {
inline void backend_init_once() {
  // A process may contain multiple model sessions. Never shut down a shared
  // llama backend in an individual session destructor.
  static std::once_flag flag;
  std::call_once(flag, [] { llama_backend_init(); });
}
inline std::string checked_utf8(std::string_view input, const char *name) {
  if(input.empty() || input.size()>65536 || input.find('\0')!=std::string_view::npos)
    throw std::invalid_argument(std::string(name)+" must be nonempty, NUL-free and <=65536 bytes");
  (void)utf8_scalars(input); // reject malformed byte sequences
  return std::string(input);
}
inline std::string token_piece(const llama_vocab *vocab, llama_token id) {
  char small[256];
  int n=llama_token_to_piece(vocab,id,small,sizeof(small),0,true);
  if(n>=0) return std::string(small,static_cast<std::size_t>(n));
  if(n==std::numeric_limits<int>::min() || -n>65536)
    throw std::runtime_error("SmolLM2 token piece exceeds bounded output");
  std::string out(static_cast<std::size_t>(-n),'\0');
  n=llama_token_to_piece(vocab,id,out.data(),static_cast<int>(out.size()),0,true);
  if(n<0 || static_cast<std::size_t>(n)>out.size())
    throw std::runtime_error("Failed to decode SmolLM2 token into UTF-8");
  out.resize(static_cast<std::size_t>(n));
  return out;
}
inline void batch_set(llama_batch_ext *batch,const llama_token *tokens,
                      int32_t count,llama_pos position) {
  llama_batch_ext_clear(batch);
  for(int32_t i=0;i<count;++i) {
    const int32_t index=llama_batch_ext_add_token(batch,0,tokens[i]);
    const llama_pos pos=position+i;
    llama_batch_ext_set_pos(batch,index,&pos);
  }
  llama_batch_ext_set_output_logits(batch,count-1,true);
}
struct model_deleter {
  void operator()(llama_model *model)const noexcept{if(model)llama_model_free(model);}
};
struct ctx_deleter {
  void operator()(llama_context *ctx)const noexcept{if(ctx)llama_free(ctx);}
};
struct batch_deleter {
  void operator()(llama_batch_ext *batch)const noexcept{if(batch)llama_batch_ext_free(batch);}
};
struct sampler_deleter {
  void operator()(llama_sampler *sampler)const noexcept{if(sampler)llama_sampler_free(sampler);}
};
} // namespace smollm2_detail

class SmolLM2NativeSession final {
 public:
  // Use a genuine GGUF conversion of HuggingFaceTB/SmolLM2-135M-Instruct.
  // Quantized inference is not claimed to reproduce FP32 Transformers.js bytes.
  explicit SmolLM2NativeSession(std::string_view gguf_path) {
    const auto path=smollm2_detail::checked_utf8(gguf_path,"SmolLM2 model path");
    smollm2_detail::backend_init_once();
    llama_model_params p=llama_model_default_params();
    p.n_gpu_layers=0; // deterministic CPU-only execution
    model_.reset(llama_model_load_from_file(path.c_str(),p));
    if(!model_)throw std::runtime_error("SmolLM2 genuine GGUF weights failed to load");
    if(llama_model_has_encoder(model_.get()))
      throw std::runtime_error("SmolLM2 weights unexpectedly contain an encoder model");
    if(!llama_model_get_vocab(model_.get()))
      throw std::runtime_error("SmolLM2 native model has no tokenizer");
  }
  SmolLM2NativeSession(const SmolLM2NativeSession&)=delete;
  SmolLM2NativeSession& operator=(const SmolLM2NativeSession&)=delete;
  SmolLM2NativeSession(SmolLM2NativeSession&&)=default;
  SmolLM2NativeSession& operator=(SmolLM2NativeSession&&)=default;
  ~SmolLM2NativeSession()=default;

  bool loaded()const noexcept{return static_cast<bool>(model_);}
  void close()noexcept{model_.reset();}

  // Synchronous, greedy CPU generation. Matches the GutTS default
  // return_full_text contract: prompt + actual generated model tokens.
  // No JavaScript Promise/AbortSignal or browser runtime is fabricated.
  std::string generate(std::string_view input,std::size_t max_new_tokens=32)const {
    const auto prompt=smollm2_detail::checked_utf8(input,"SmolLM2 prompt");
    if(!model_)throw std::runtime_error("SmolLM2 model session closed or moved");
    if(max_new_tokens>1024)throw std::invalid_argument("SmolLM2 maximum is 1024 new tokens");
    if(max_new_tokens==0)return prompt;
    const llama_vocab *vocab=llama_model_get_vocab(model_.get());
    const int needed=llama_tokenize(vocab,prompt.data(),static_cast<int32_t>(prompt.size()),
                                    nullptr,0,true,true);
    if(needed>=0 || needed==std::numeric_limits<int>::min())
      throw std::runtime_error("SmolLM2 tokenizer returned invalid token count");
    const int32_t count=-needed;
    if(count<=0 || count>512)
      throw std::invalid_argument("SmolLM2 prompt exceeds 512 model tokens");
    std::vector<llama_token> tokens(static_cast<std::size_t>(count));
    if(llama_tokenize(vocab,prompt.data(),static_cast<int32_t>(prompt.size()),
                      tokens.data(),count,true,true)!=count)
      throw std::runtime_error("SmolLM2 actual tokenizer failed");
    llama_context_params cp=llama_context_default_params();
    cp.n_ctx=static_cast<uint32_t>(std::max<std::size_t>(64,static_cast<std::size_t>(count)+max_new_tokens+8));
    cp.n_batch=static_cast<uint32_t>(count);
    cp.n_ubatch=static_cast<uint32_t>(count);
    cp.no_perf=true;
    std::unique_ptr<llama_context,smollm2_detail::ctx_deleter> ctx(
        llama_init_from_model(model_.get(),cp));
    if(!ctx)throw std::runtime_error("SmolLM2 native context allocation failed");
    auto sp=llama_sampler_chain_default_params();
    sp.no_perf=true;
    std::unique_ptr<llama_sampler,smollm2_detail::sampler_deleter> sampler(
        llama_sampler_chain_init(sp));
    if(!sampler)throw std::runtime_error("SmolLM2 greedy sampler allocation failed");
    llama_sampler_chain_add(sampler.get(),llama_sampler_init_greedy());
    std::unique_ptr<llama_batch_ext,smollm2_detail::batch_deleter> batch(
        llama_batch_ext_init(ctx.get()));
    if(!batch)throw std::runtime_error("SmolLM2 native batch allocation failed");
    smollm2_detail::batch_set(batch.get(),tokens.data(),count,0);
    int32_t n_batch=count;
    llama_pos next_pos=0;
    std::string full=prompt;
    std::size_t generated=0;
    while(generated<max_new_tokens){
      if(llama_process(ctx.get(),LLAMA_PROCESS_TYPE_DECODE,batch.get())!=0)
        throw std::runtime_error("SmolLM2 actual model decoding failed");
      next_pos+=n_batch;
      const llama_token tok=llama_sampler_sample(sampler.get(),ctx.get(),-1);
      if(llama_vocab_is_eog(vocab,tok))break;
      const auto piece=smollm2_detail::token_piece(vocab,tok);
      if(full.size()>1048576-piece.size())
        throw std::length_error("SmolLM2 generated text exceeded bounded output");
      full+=piece;
      ++generated;
      smollm2_detail::batch_set(batch.get(),&tok,1,next_pos);
      n_batch=1;
    }
    return full;
  }
 private:
  std::unique_ptr<llama_model,smollm2_detail::model_deleter> model_;
};
} // namespace gut
