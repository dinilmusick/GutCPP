#include "gut/semantic_workflow.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#define CHECK(x) do {if(!(x)){std::cerr<<"semantic_workflow_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
using namespace gut;
int main(){
  int calls=0;std::vector<std::string> events;
  semantic_workflow_request request;
  request.input=" TRUE ";request.schema.type="boolean";request.high_risk=true;request.preserve_raw_input=true;
  request.on_event=[&](const std::string& x){events.push_back(x);};
  semantic_workflow_models models;
  models.complete=[&](std::string_view,const json_value&)->std::optional<json_value>{++calls;return json_value("not used");};
  auto result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="resolved"&&result.deterministic&&result.value==json_value(true));
  CHECK(result.requires_confirmation&&result.raw_input==request.input&&calls==0);
  CHECK(std::find(events.begin(),events.end(),"semantic.deterministic")!=events.end());
  request.input=" \t ";result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="invalid"&&result.error_code=="EMPTY_INPUT");
  request.input="categorize this";request.schema.type="object";request.cancelled=[](){return true;};
  result=resolve_semantic_workflow(request,models);CHECK(result.status=="failed"&&result.error_code=="ABORTED");
  request.cancelled={};result=resolve_semantic_workflow(request);
  CHECK(result.status=="failed"&&result.error_code=="MODEL_FAILURE");
  request.input="first option";request.schema.type="string";request.candidates={"option-a","option-b"};
  models.similarity=[](std::string_view,std::string_view item){return item=="option-a"?.9:.5;};
  models.rerank=[](std::string_view,const std::vector<std::string>& values){if(values.size()!=2)throw std::runtime_error("unexpected cardinality");return std::vector<double>{.91,.4};};
  models.complete=[&](std::string_view prompt,const json_value& schema)->std::optional<json_value>{
    ++calls;CHECK(prompt.find("first option")!=std::string_view::npos);
    CHECK(std::holds_alternative<json_value::object>(schema.data));
    return json_value(json_value::object{{"output",json_value("something unknown")}});
  };
  semantic_workflow_cache cache(2);
  request.cache=&cache;
  result=resolve_semantic_workflow(request,models);
  CHECK(cache.size()==1);
  const auto prior_calls=calls;
  auto from_cache=resolve_semantic_workflow(request,models);
  CHECK(from_cache.status=="resolved"&&calls==prior_calls);
  CHECK(result.status=="resolved"&&result.value==json_value("option-a"));
  CHECK(result.selected_candidate=="option-a"&&result.candidates.size()==2);
  request.minimum_similarity=.98;
  result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="ambiguous"&&result.error_code=="AMBIGUOUS_CANDIDATE"&&result.clarification);
  models.complete=[](std::string_view,const json_value&)->std::optional<json_value>{return json_value("option-b");};
  request.model_revision="model-v2"; // invalidate prior ranking/inference cache
  result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="resolved"&&result.selected_candidate=="option-b");
  request.candidates.clear();request.input="structure this";request.model_revision="model-v3";
  models.complete=[](std::string_view,const json_value&)->std::optional<json_value>{return json_value(json_value::object{{"data",json_value(json_value::object{{"foo",1}})}});};
  result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="resolved"&&result.value==json_value(json_value::object{{"foo",1}}));
  request.model_revision="model-v4";
  models.complete=[](std::string_view,const json_value&)->std::optional<json_value>{throw std::runtime_error("model offline");};
  result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="failed"&&result.error_code=="MODEL_FAILURE");
  request.model_revision="model-v5";
  models.complete=[](std::string_view,const json_value&)->std::optional<json_value>{return std::nullopt;};
  result=resolve_semantic_workflow(request,models);
  CHECK(result.status=="unresolved"&&result.error_code=="MODEL_OUTPUT_EMPTY");
}
