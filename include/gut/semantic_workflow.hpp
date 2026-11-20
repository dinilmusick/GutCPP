#pragma once
// Full *orchestration* path for GutTS semantic-input, with mandatory real
// caller-owned model providers. This does not implement model inference.
#include "gut/semantic_input.hpp"
#include <algorithm>
#include <list>
#include <mutex>
#include <unordered_map>
#include <variant>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
class semantic_workflow_cache;
struct semantic_ranked_candidate {
  std::string value;
  double similarity=0.0;
  double relevance=0.0;
};
struct semantic_workflow_models {
  // Supply actual MiniLM, Ettin and Needle model calls here. Missing callbacks
  // fail closed; synthetic embeddings or fabricated inference are forbidden.
  std::function<double(std::string_view,std::string_view)> similarity;
  std::function<std::vector<double>(std::string_view,const std::vector<std::string>&)> rerank;
  std::function<std::optional<json_value>(std::string_view,const json_value&)> complete;
};
struct semantic_workflow_request {
  std::string input;
  semantic_input_schema schema;
  json_value schema_wire=json_value(json_value::object{});
  std::vector<std::string> candidates;
  std::vector<std::string> vocabulary;
  std::string field_label="this field";
  std::string model_revision; // caller updates on model/provider/config change
  bool preserve_raw_input=false;
  bool requires_confirmation=false;
  bool high_risk=false;
  double minimum_similarity=0.65;
  double minimum_relevance_margin=0.15;
  std::function<bool()> cancelled;
  std::function<void(const std::string&)> on_event;
  std::function<void(const std::string&)> on_progress;
  semantic_workflow_cache* cache=nullptr; // caller-owned, never global
};
struct semantic_workflow_resolution {
  std::string status;
  std::optional<json_value> value;
  std::optional<json_value> model_output;
  std::vector<semantic_ranked_candidate> candidates;
  std::optional<std::string> selected_candidate;
  std::optional<std::string> raw_input;
  std::optional<std::string> error_code;
  std::optional<std::string> error_message;
  std::optional<std::string> clarification;
  bool requires_confirmation=false;
  bool deterministic=false;
};
// Owns bounded complete resolution records; synchronizes accesses to avoid
// unsafe stale iterators under concurrent native UI/request threads.
class semantic_workflow_cache {
  using entry=std::pair<std::string,semantic_workflow_resolution>;
  const std::size_t capacity_;
  std::mutex mutex_;
  std::list<entry> entries_;
  std::unordered_map<std::string,typename std::list<entry>::iterator> index_;
 public:
  explicit semantic_workflow_cache(std::size_t max_entries=128):capacity_(max_entries){}
  semantic_workflow_cache(const semantic_workflow_cache&)=delete;
  semantic_workflow_cache& operator=(const semantic_workflow_cache&)=delete;
  std::optional<semantic_workflow_resolution> get(const std::string& key){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it=index_.find(key);if(it==index_.end())return std::nullopt;
    entries_.splice(entries_.end(),entries_,it->second);
    return it->second->second;
  }
  void set(std::string key,semantic_workflow_resolution value){
    std::lock_guard<std::mutex> lock(mutex_);
    auto it=index_.find(key);if(it!=index_.end()){entries_.erase(it->second);index_.erase(it);}
    if(!capacity_)return;
    entries_.emplace_back(std::move(key),std::move(value));
    auto current=std::prev(entries_.end());index_[current->first]=current;
    while(entries_.size()>capacity_){index_.erase(entries_.front().first);entries_.pop_front();}
  }
  void clear(){std::lock_guard<std::mutex> lock(mutex_);entries_.clear();index_.clear();}
  std::size_t size(){std::lock_guard<std::mutex> lock(mutex_);return entries_.size();}
};
namespace semantic_workflow_detail {
inline void emit(const std::function<void(const std::string&)>& cb,const std::string& event){if(cb)cb(event);}
inline bool aborted(const semantic_workflow_request& r){return r.cancelled&&r.cancelled();}
inline std::string cache_key(const semantic_workflow_request& r){
  std::string key;
  const auto append=[&](std::string_view value){key+=std::to_string(value.size());key+=':';key.append(value.data(),value.size());};
  append(r.input);append(r.schema.type);append(r.schema.format.value_or(""));append(r.schema.pattern.value_or(""));
  append(stringify_json(r.schema_wire));
  append(stringify_json(json_value(json_value::array(r.schema.allowed))));
  for(const auto& value:r.candidates)append(value);
  append("candidates-end");for(const auto& value:r.vocabulary)append(value);
  append(r.field_label);append(r.model_revision);append(std::to_string(r.minimum_similarity));append(std::to_string(r.minimum_relevance_margin));
  append(r.preserve_raw_input?"raw":"noraw");append(r.requires_confirmation?"confirm":"noconfirm");
  append(r.high_risk?"high":"ordinary");
  return key;
}
inline semantic_workflow_resolution failure(const semantic_workflow_request& r,std::string status,std::string code,
      std::string message,std::vector<semantic_ranked_candidate> candidates={}) {
  semantic_workflow_resolution result;
  result.status=std::move(status);result.error_code=std::move(code);result.error_message=std::move(message);
  result.candidates=std::move(candidates);
  if(r.preserve_raw_input)result.raw_input=r.input;
  if(result.status=="failed"||result.status=="invalid")
    emit(r.on_event,std::string("semantic.")+result.status+":"+*result.error_code);
  return result;
}
inline std::optional<json_value> unwrap(std::optional<json_value> raw) {
  if(!raw)return std::nullopt;
  if(const auto* record=std::get_if<json_value::object>(&raw->data)){
    for(const auto& key:{"output","result","data","structuredOutput"}){
      const auto it=record->find(key);if(it!=record->end())return it->second;
    }
  }
  return raw;
}
inline std::string prompt(const semantic_workflow_request& r,const std::vector<std::string>& candidates) {
  std::string text="Convert the natural-language input into the supplied target schema.\n"
   "Return only data conforming to the target schema. Never invent information.\n";
  if(!r.field_label.empty())text+="Field: "+r.field_label+"\n";
  if(!candidates.empty()) {
    text+="Candidate values: ";for(std::size_t i=0;i<candidates.size();++i){if(i)text+=" | ";text+=candidates[i];}text+='\n';
  }
  text+="Target schema: "+stringify_json(r.schema_wire)+"\nInput: "+r.input;
  return text;
}
inline std::vector<semantic_ranked_candidate> rank(std::string_view input,
 const std::vector<std::string>& candidates,const semantic_workflow_models& models) {
  if(candidates.empty())return {};
  if(!models.similarity||!models.rerank)throw std::runtime_error("MiniLM and Ettin runtime providers are required");
  const auto relevance=models.rerank(input,candidates);
  if(relevance.size()!=candidates.size())throw std::runtime_error("Ettin returned wrong candidate count");
  std::vector<semantic_ranked_candidate> out;out.reserve(candidates.size());
  for(std::size_t i=0;i<candidates.size();++i){
    const auto sim=models.similarity(input,candidates[i]);
    if(!std::isfinite(sim)||!std::isfinite(relevance[i]))throw std::runtime_error("Nonfinite model rank score");
    out.push_back({candidates[i],sim,relevance[i]});
  }
  std::stable_sort(out.begin(),out.end(),[](const auto& a,const auto& b){
    if(a.relevance!=b.relevance)return a.relevance>b.relevance;
    if(a.similarity!=b.similarity)return a.similarity>b.similarity;
    return a.value<b.value;
  });
  return out;
}
} // namespace semantic_workflow_detail
// Synchronous native counterpart: callbacks may themselves call actual model
// backends, but this function neither spawns nor schedules agents or workers.
inline semantic_workflow_resolution resolve_semantic_workflow(const semantic_workflow_request& request,
  const semantic_workflow_models& models={}) {
  using namespace semantic_workflow_detail;
  emit(request.on_progress,"started");emit(request.on_event,"semantic.started");
  if(aborted(request))return failure(request,"failed","ABORTED","Semantic resolution was aborted.");
  const auto cache_key_value=request.cache?cache_key(request):std::string{};
  if(request.cache)if(auto cached=request.cache->get(cache_key_value))return *cached;
  if(semantic_detail::trim(request.input).empty()){
    auto invalid=failure(request,"invalid","EMPTY_INPUT","Semantic input is empty.");
    if(request.cache)request.cache->set(cache_key_value,invalid);
    return invalid;
  }
  const auto deterministic=resolve_deterministic_semantic_input(request.input,request.schema);
  if(deterministic){
    semantic_workflow_resolution r;
    r.status="resolved";r.deterministic=true;r.value=deterministic->value;r.model_output=r.value;
    r.requires_confirmation=request.requires_confirmation||request.high_risk;
    if(request.preserve_raw_input)r.raw_input=request.input;
    emit(request.on_progress,"deterministic");emit(request.on_event,"semantic.deterministic");
    emit(request.on_event,"semantic.resolved");
    if(request.cache)request.cache->set(cache_key_value,r);
    return r;
  }
  const auto& options=!request.candidates.empty()?request.candidates:request.vocabulary;
  std::vector<semantic_ranked_candidate> ranked;
  emit(request.on_progress,"ranking");
  try{ranked=rank(request.input,options,models);}catch(const std::exception&){
    return failure(request,"failed","MODEL_FAILURE","Candidate ranking failed.");
  }
  if(aborted(request))return failure(request,"failed","ABORTED","Semantic resolution was aborted.",std::move(ranked));
  if(!models.complete)return failure(request,"failed","MODEL_FAILURE","Needle model provider is not configured.",std::move(ranked));
  emit(request.on_progress,"structured");
  std::optional<json_value> model_output;
  try{model_output=unwrap(models.complete(prompt(request,options),request.schema_wire));}
  catch(const std::exception&){return failure(request,"failed","MODEL_FAILURE","Needle structured resolution failed.",std::move(ranked));}
  if(!model_output||std::holds_alternative<std::nullptr_t>(model_output->data)||
     (std::holds_alternative<std::string>(model_output->data)&&std::get<std::string>(model_output->data).empty())) {
    auto result=failure(request,"unresolved","MODEL_OUTPUT_EMPTY","Needle returned no usable semantic value.",std::move(ranked));
    result.model_output=model_output;return result;
  }
  semantic_workflow_resolution result;
  result.model_output=model_output;result.candidates=std::move(ranked);
  if(request.preserve_raw_input)result.raw_input=request.input;
  if(!result.candidates.empty()&&std::holds_alternative<std::string>(model_output->data)) {
    const auto& output=std::get<std::string>(model_output->data);
    auto exact=std::find_if(result.candidates.begin(),result.candidates.end(),[&](const auto& c){return c.value==output;});
    if(exact!=result.candidates.end())result.selected_candidate=exact->value;
    else {
      const auto& top=result.candidates.front();
      const auto margin=result.candidates.size()<2?std::numeric_limits<double>::infinity():
        top.relevance-result.candidates[1].relevance;
      if(top.similarity>=request.minimum_similarity&&margin>=request.minimum_relevance_margin)
        result.selected_candidate=top.value;
    }
    if(result.selected_candidate)result.value=json_value(*result.selected_candidate);
  } else result.value=model_output;
  if(!result.value){
    result.status=result.candidates.empty()?"invalid":"ambiguous";
    result.error_code=result.candidates.empty()?"SCHEMA_MISMATCH":"AMBIGUOUS_CANDIDATE";
    result.error_message=result.candidates.empty()?"Model output could not be resolved to target schema.":
      "Semantic evidence did not uniquely identify a candidate.";
    if(result.candidates.size()>1){
      result.clarification="Which value did you mean for "+request.field_label+": ";
      const auto count=std::min<std::size_t>(3,result.candidates.size());
      for(std::size_t i=0;i<count;++i){if(i)result.clarification->append(", ");result.clarification->append(result.candidates[i].value);}
      result.clarification->append("?");
      emit(request.on_event,"semantic.clarification.required");
    }
    emit(request.on_event,"semantic."+result.status);
    if(request.cache)request.cache->set(cache_key_value,result);
    return result;
  }
  result.status="resolved";result.requires_confirmation=request.requires_confirmation||request.high_risk;
  emit(request.on_progress,"completed");emit(request.on_event,"semantic.resolved");
  if(request.cache)request.cache->set(cache_key_value,result);
  return result;
}
} // namespace gut
