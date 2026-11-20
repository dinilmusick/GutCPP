#pragma once
// Pinned GutTS model runtime MiniLM post-embedding math, C++17.
// Consumes real Float32Array-compatible pooled model embeddings. This header
// does NOT load an ML model or substitute fake embeddings for model inference.
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace gut {
// Exact scalar operation order of GutTS runtime transformers.ts cosine().
// A missing right-hand element is zero, matching JS `right[i] ?? 0`.
inline double minilm_cosine(const std::vector<float>& left,
                            const std::vector<float>& right) noexcept {
  double dot=0.0,left_norm=0.0,right_norm=0.0;
  for(std::size_t i=0;i<left.size();++i){
    const double l=static_cast<double>(left[i]);
    const double r=i<right.size()?static_cast<double>(right[i]):0.0;
    dot+=l*r;
    left_norm+=l*l;
    right_norm+=r*r;
  }
  return dot/std::sqrt(left_norm*right_norm);
}

// The compiled GutTS MiniLM runtime calls a normalized feature-extraction
// pipeline on [left,right], then splits the returned Float32Array in half.
// Calling this function on actual pipeline embeddings applies the same math.
inline double minilm_similarity_from_pooled_embeddings(const std::vector<float>& pooled) {
  const auto half=pooled.size()/2;
  if(half==0||pooled.size()%2!=0)
    throw std::invalid_argument("MiniLM runtime returned invalid embeddings.");
  const std::vector<float> left(pooled.begin(),pooled.begin()+static_cast<std::ptrdiff_t>(half));
  const std::vector<float> right(pooled.begin()+static_cast<std::ptrdiff_t>(half),pooled.end());
  return minilm_cosine(left,right);
}
} // namespace gut
