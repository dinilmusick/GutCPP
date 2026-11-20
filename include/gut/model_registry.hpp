#pragma once
// Immutable metadata for the eight models published by pinned GutTS.
// Data only; a metadata record does not implement model inference.
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gut {
struct model_size {
  double value=0;
  std::optional<double> min;
  std::optional<double> max;
  std::string unit;
  std::string qualifier;
};
struct model_metadata {
  std::string id;
  std::string name;
  std::string provider;
  std::string version;
  std::string description;
  std::vector<std::string> examples;
  std::string when_to_use;
  std::string avoid_when;
  model_size size;
  std::vector<std::string> capabilities;
  std::string input;
  std::string output;
  std::string runtime;
  std::optional<std::string> model_uri;
  std::optional<std::string> license;
};
inline const std::vector<model_metadata>& gutts_model_definitions() {
 static const std::vector<model_metadata> values=[] {
  std::vector<model_metadata> models;
  models.push_back({"model.smollm2-135m-instruct","SmolLM2-135M-Instruct","Hugging Face","135M",
    "Compact instruction-tuned language model for general text generation.",
    {"summarize text","rewrite text","draft a short response"},
    "Use for short general-purpose text generation when a local model is preferred.",
    "Avoid for high-stakes reasoning, broad factual knowledge, or tasks requiring large context.",
    {105,{}, {},"MB","artifact"},{"text-generation","instruction-following"},
    "Natural-language prompt","Generated natural-language text","ModelRuntime adapter",
    "HuggingFaceTB/SmolLM2-135M-Instruct","Apache-2.0"});
  models.push_back({"model.cactus-needle3","Needle 3","Cactus Compute","3",
    "Tiny structured inference model for tool selection, argument extraction, classification, and embeddings.",
    {"select a tool","extract fields","classify an intent"},
    "Use when natural language must be mapped to a small tool set or structured schema.",
    "Avoid for open-ended long-form generation or complex multi-step reasoning.",
    {0,8,29,"MB","range"},{"tool-selection","argument-extraction","classification","structured-output","embeddings"},
    "Natural-language text with optional tools or extraction schema","Structured JSON or embedding vector",
    "Cactus Needle runtime adapter","Cactus-Compute/needle3",{}});
  models.push_back({"model.cactus-whistle","Whistle","Cactus Compute","1",
    "Compact CPU speech-recognition model for on-device transcription.",
    {"transcribe a voice note","transcribe a command"},
    "Use for local speech-to-text when low resource use and privacy matter.",
    "Avoid when audio understanding beyond transcription is required.",
    {16.9,{}, {},"MB","artifact"},{"speech-to-text","on-device","cpu"},
    "Audio bytes","Transcribed natural-language text","Cactus Needle runtime adapter",
    "Cactus-Compute/whistle",{}});
  const auto ettin=[&](const std::string& id,const std::string& name,int millions,const std::string& description){
    models.push_back({id,name,"Hugging Face",std::to_string(millions),description,
      {"rank search results","order retrieved documents by relevance"},
      "Use for cross-encoder reranking when candidate texts must be sorted by semantic relevance.",
      "Avoid for text generation, embeddings, or transcription.",
      {static_cast<double>(millions)*1e6,{}, {},"parameters","parameters"},
      {"reranking","relevance-scoring"},"Query and candidate texts","Candidate relevance scores",
      "ModelRuntime adapter","cross-encoder/ettin-reranker-"+std::to_string(millions)+"m-v1","Apache-2.0"});
  };
  ettin("model.ettin-reranker-17m","Ettin-Reranker-17M",17,"Very small relevance model for ranking candidate text.");
  ettin("model.ettin-reranker-32m","Ettin-Reranker-32M",32,"Small relevance model for ranking candidate text.");
  ettin("model.ettin-reranker-68m","Ettin-Reranker-68M",68,"Compact relevance model for higher-quality candidate ranking.");
  ettin("model.ettin-reranker-150m","Ettin-Reranker-150M",150,"Larger compact relevance model for candidate ranking.");
  models.push_back({"model.minilm-l6-v2","MiniLM-L6-v2","Sentence Transformers","L6-v2",
    "Compact sentence embedding model for semantic similarity.",
    {"compare two texts","match a query to a short description"},
    "Use for lightweight semantic similarity and embedding-based matching.",
    "Avoid when generation, extraction, or tool calling is required.",
    {22700000,{}, {},"parameters","parameters"},{"embeddings","semantic-similarity"},
    "Two text strings","Similarity score","ModelRuntime adapter",
    "sentence-transformers/all-MiniLM-L6-v2","Apache-2.0"});
  return models;
 }();
 return values;
}
inline const model_metadata* gutts_model_by_id(std::string_view id) {
 const auto& models=gutts_model_definitions();
 const auto it=std::find_if(models.begin(),models.end(),[&](const auto& model){return model.id==id;});
 return it==models.end()?nullptr:&*it;
}
} // namespace gut
