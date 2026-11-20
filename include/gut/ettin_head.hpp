#pragma once
// Actual portable Ettin scoring head from the pinned GutTS runtimes/ettin-head.ts.
// Accepts real float32 transformer hidden states and Safetensors head weights.
// Does not fabricate backbone embeddings, perform tokenizer inference or schedule work.
#include "gut/json.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gut {
struct ettin_head_weights {
  std::vector<float> w1, ln_weight, ln_bias, w2, b2;
};
struct ettin_hidden_states {
  std::size_t batch_size=0, sequence_length=0, hidden_size=0;
  std::vector<float> data;
};
namespace ettin_detail {
inline std::size_t checked_multiply(std::size_t a,std::size_t b) {
  if(b && a>std::numeric_limits<std::size_t>::max()/b)
    throw std::overflow_error("Ettin tensor dimensions overflow");
  return a*b;
}
inline std::size_t offset(const json_value& item,std::size_t payload_size) {
  const double n=item.as<double>();
  if(!std::isfinite(n)||n<0||std::floor(n)!=n||n>static_cast<double>(payload_size))
    throw std::invalid_argument("Safetensors offset out of range");
  return static_cast<std::size_t>(n);
}
inline std::uint64_t le64(std::string_view bytes){
  std::uint64_t n=0;
  for(unsigned i=0;i<8;++i)n|=static_cast<std::uint64_t>(static_cast<unsigned char>(bytes[i]))<<(8*i);
  return n;
}
inline float le_float32(const unsigned char* bytes){
  static_assert(sizeof(float)==sizeof(std::uint32_t),"Ettin requires IEEE 754 float32");
  std::uint32_t bits=0;
  for(unsigned i=0;i<4;++i)bits|=static_cast<std::uint32_t>(bytes[i])<<(8*i);
  float value=0;
  std::memcpy(&value,&bits,sizeof(value));
  return value;
}
inline std::string read_file(const std::string& path) {
  std::ifstream input(path,std::ios::in|std::ios::binary);
  if(!input)throw std::runtime_error("Failed to open Ettin head weights: "+path);
  std::string bytes((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
  if(!input.eof() && input.bad())throw std::runtime_error("Failed to read Ettin head weights: "+path);
  return bytes;
}
inline void check_head(const ettin_hidden_states& h,const ettin_head_weights& w) {
  const auto count=checked_multiply(h.batch_size,checked_multiply(h.sequence_length,h.hidden_size));
  if(count!=h.data.size()||(h.batch_size>0&&(h.sequence_length==0||h.hidden_size==0)))
    throw std::invalid_argument("Ettin hidden-state tensor shape mismatch");
  const auto squares=checked_multiply(h.hidden_size,h.hidden_size);
  if(w.w1.size()!=squares||w.ln_weight.size()!=h.hidden_size||w.ln_bias.size()!=h.hidden_size||
     w.w2.size()!=h.hidden_size||w.b2.size()!=1)
    throw std::invalid_argument("Ettin head tensor shape mismatch");
  for(const auto* vec:{&h.data,&w.w1,&w.ln_weight,&w.ln_bias,&w.w2,&w.b2})
    for(float val:*vec)if(!std::isfinite(val))throw std::invalid_argument("Non-finite Ettin tensor value");
}
} // namespace ettin_detail

// Parse real F32 Safetensors bytes (8-byte little-endian JSON length, JSON
// metadata, then little-endian IEEE 754 tensor payload). Bounds are checked.
inline std::vector<float> parse_safetensors_f32(std::string_view bytes,std::string_view tensor_name) {
  if(bytes.size()<8||!std::numeric_limits<float>::is_iec559)
    throw std::invalid_argument("Invalid F32 Safetensors buffer/platform");
  const std::uint64_t header_size=ettin_detail::le64(bytes.substr(0,8));
  if(header_size>bytes.size()-8||header_size>4*1024*1024)
    throw std::invalid_argument("Invalid Safetensors header size");
  const std::size_t payload_start=8+static_cast<std::size_t>(header_size);
  const auto metadata=parse_json(bytes.substr(8,static_cast<std::size_t>(header_size)));
  const auto& root=metadata.as<json_value::object>();
  const auto it=root.find(std::string(tensor_name));
  if(it==root.end())throw std::invalid_argument("Missing Safetensors head tensor");
  const auto& record=it->second.as<json_value::object>();
  const auto dtype=record.find("dtype");
  if(dtype!=record.end() && dtype->second.as<std::string>()!="F32")
    throw std::invalid_argument("Ettin requires F32 head tensor");
  const auto& offsets=record.at("data_offsets").as<json_value::array>();
  if(offsets.size()!=2)throw std::invalid_argument("Invalid Safetensors data_offsets");
  const auto start=ettin_detail::offset(offsets[0],bytes.size()-payload_start);
  const auto end=ettin_detail::offset(offsets[1],bytes.size()-payload_start);
  if(end<start || (end-start)%sizeof(float)!=0)
    throw std::invalid_argument("Invalid Safetensors float range");
  const auto* data=reinterpret_cast<const unsigned char*>(bytes.data()+payload_start+start);
  std::vector<float> values;
  values.reserve((end-start)/sizeof(float));
  for(std::size_t pos=0;pos<end-start;pos+=4)values.push_back(ettin_detail::le_float32(data+pos));
  return values;
}
inline ettin_head_weights load_ettin_head_weights(const std::string& model_directory) {
  const auto dense2=ettin_detail::read_file(model_directory+"/2_Dense/model.safetensors");
  const auto norm=ettin_detail::read_file(model_directory+"/3_LayerNorm/model.safetensors");
  const auto dense4=ettin_detail::read_file(model_directory+"/4_Dense/model.safetensors");
  return {
    parse_safetensors_f32(dense2,"linear.weight"),
    parse_safetensors_f32(norm,"norm.weight"),
    parse_safetensors_f32(norm,"norm.bias"),
    parse_safetensors_f32(dense4,"linear.weight"),
    parse_safetensors_f32(dense4,"linear.bias"),
  };
}
// This is the real dense->GELU->layer-norm->logistic scoring head. The input
// must be actual transformer last_hidden_state; no mock inference is performed.
inline std::vector<double> compute_ettin_head_scores(const ettin_hidden_states& hidden,
                                                     const ettin_head_weights& head) {
  ettin_detail::check_head(hidden,head);
  std::vector<double> scores;scores.reserve(hidden.batch_size);
  const std::size_t width=hidden.hidden_size;
  const double c=std::sqrt(2.0/3.141592653589793238462643383279502884);
  for(std::size_t batch=0;batch<hidden.batch_size;++batch) {
    const auto cls_offset=ettin_detail::checked_multiply(batch,ettin_detail::checked_multiply(hidden.sequence_length,width));
    const float* cls=hidden.data.data()+cls_offset;
    std::vector<float> y(width);
    for(std::size_t i=0;i<width;++i){
      double sum=0;
      const auto row=ettin_detail::checked_multiply(i,width);
      for(std::size_t j=0;j<width;++j)sum+=static_cast<double>(head.w1[row+j])*cls[j];
      y[i]=static_cast<float>(0.5*sum*(1+std::tanh(c*(sum+0.044715*sum*sum*sum))));
    }
    double mean=0;
    for(float value:y)mean+=value;
    mean/=static_cast<double>(width);
    double variance=0;
    for(float value:y){const double delta=value-mean;variance+=delta*delta;}
    const double inv_std=1.0/std::sqrt(variance/static_cast<double>(width)+1e-5);
    for(std::size_t i=0;i<width;++i)
      y[i]=static_cast<float>((static_cast<double>(y[i])-mean)*inv_std*head.ln_weight[i]+head.ln_bias[i]);
    double logit=head.b2[0];
    for(std::size_t i=0;i<width;++i)logit+=static_cast<double>(head.w2[i])*y[i];
    scores.push_back(1.0/(1.0+std::exp(-logit)));
  }
  return scores;
}
} // namespace gut
