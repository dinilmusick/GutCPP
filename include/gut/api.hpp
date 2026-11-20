#pragma once
// Executable portable subset of GutTS API/FOD registry. IDs match the pinned TS registry.
// Metadata-only model entries are not advertised as executable without a real provider.
#include "gut/json.hpp"
#include "gut/net.hpp"
#include "gut/strings.hpp"
#include "gutcpp/url.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace gut {
struct api_parameter {
  std::string name;
  bool required = true;
  std::string json_type;
};
struct api_definition {
  std::string id;
  std::string name;
  std::string category;
  std::vector<api_parameter> parameters;
  bool deterministic = true;
  bool pure = true;
  bool side_effects = false;
  std::string output_type = "any";
};
struct fod_port {
  std::string id;
  std::string direction;
  std::string data_type;
  bool required = true;
};
struct fod_node_definition {
  std::string type;
  std::string label;
  std::string category="gutts";
  std::string group;
  std::vector<fod_port> inputs;
  std::vector<fod_port> outputs;
  std::string api_id;
};
inline fod_node_definition gutts_api_to_fod_node(const api_definition& api) {
  fod_node_definition node;
  node.type="gutts/"+api.id;node.label=api.name;node.group=api.category;node.api_id=api.id;
  for(const auto& p:api.parameters)node.inputs.push_back({p.name,"input",p.json_type,p.required});
  node.outputs.push_back({"result","output",api.output_type,true});
  return node;
}
inline const std::vector<api_definition>& portable_api_definitions() {
  static const std::vector<api_definition> defs=[] {
    std::vector<api_definition> out;
    const auto add=[&](const std::string& category,const std::string& name,
      std::initializer_list<api_parameter> params) {
      std::string output="any";
      if(category=="arrays")output=name=="last"?"any":"array";
      else if(category=="json")output=name=="isJsonValue"?"boolean":name=="stringifyJson"?"string":name=="parseJson"?"object":"any";
      else if(category=="numbers")output=name=="isFiniteNumber"||name=="inRange"?"boolean":"number";
      else if(category=="objects")output=name=="hasOwn"||name=="isPlainObject"?"boolean":"object";
      else if(category=="strings")output=name=="isBlank"||name=="isNonBlank"||name=="isEmail"||name=="startsWithAny"||name=="endsWithAny"?"boolean":"string";
      else if(category=="urls")output=name=="isUrl"?"boolean":"string";
      out.push_back({"gutts."+category+"."+name,name,category,params,true,true,false,std::move(output)});
    };
    add("arrays","compact",{{"values",true,"array"}});
    add("arrays","unique",{{"values",true,"array"}});
    add("arrays","chunk",{{"values",true,"array"},{"size",true,"integer"}});
    add("arrays","flatten",{{"values",true,"array"}});
    add("arrays","last",{{"values",true,"array"}});
    add("json","isJsonValue",{{"value",true,"any"}});
    add("json","parseJson",{{"text",true,"string"}});
    add("json","tryParseJson",{{"text",true,"string"}});
    add("json","stringifyJson",{{"value",true,"any"},{"space",false,"number"}});
    add("numbers","clamp",{{"value",true,"number"},{"min",true,"number"},{"max",true,"number"}});
    add("numbers","inRange",{{"value",true,"number"},{"min",true,"number"},{"max",true,"number"}});
    add("numbers","roundTo",{{"value",true,"number"},{"decimals",false,"integer"}});
    add("numbers","isFiniteNumber",{{"value",true,"any"}});
    add("numbers","sum",{{"values",true,"array"}});
    add("objects","hasOwn",{{"value",true,"object"},{"key",true,"string"}});
    add("objects","pick",{{"value",true,"object"},{"keys",true,"array"}});
    add("objects","omit",{{"value",true,"object"},{"keys",true,"array"}});
    add("objects","isPlainObject",{{"value",true,"any"}});
    add("strings","isBlank",{{"value",true,"string"}});
    add("strings","isNonBlank",{{"value",true,"string"}});
    add("strings","isEmail",{{"value",true,"string"}});
    add("strings","startsWithAny",{{"value",true,"string"},{"prefixes",true,"array"}});
    add("strings","endsWithAny",{{"value",true,"string"},{"suffixes",true,"array"}});
    add("strings","capitalize",{{"value",true,"string"}});
    add("strings","camelCase",{{"value",true,"string"}});
    add("strings","kebabCase",{{"value",true,"string"}});
    add("strings","snakeCase",{{"value",true,"string"}});
    add("strings","truncate",{{"value",true,"string"},{"maxLength",true,"number"},{"suffix",false,"string"}});
    add("strings","truncateWords",{{"value",true,"string"},{"maxWords",true,"number"},{"suffix",false,"string"}});
    add("urls","isUrl",{{"value",true,"string"}});
    add("urls","joinUrl",{{"base",true,"string"},{"parts",true,"array"}});
    add("urls","getQueryParam",{{"url",true,"string"},{"key",true,"string"}});
    add("urls","setQueryParam",{{"url",true,"string"},{"key",true,"string"},{"value",true,"string"}});
    return out;
  }();
  return defs;
}
// Published GutTS API registry includes five ML operations whose metadata is
// portable but whose native inference requires a real, installed model backend.
// Include their exact public contracts without manufacturing model results.
inline const std::vector<api_definition>& gutts_api_definitions() {
  static const std::vector<api_definition> defs=[] {
    std::vector<api_definition> out=portable_api_definitions();
    out.push_back({"model.smollm2-135m-instruct.generate","SmolLM2-135M-Instruct.generate","models",
      {{"prompt",true,"string"},{"maxNewTokens",false,"number"}},false,false,false,"string"});
    out.push_back({"model.cactus-needle3.complete","Needle3.complete","models",
      {{"prompt",true,"string"},{"tools",false,"array"},{"schema",false,"object"}},false,false,false,"object"});
    out.push_back({"model.cactus-whistle.transcribe","Whistle.transcribe","models",
      {{"audio",true,"string"},{"language",false,"string"}},true,false,false,"string"});
    out.push_back({"model.ettin-reranker.rerank","EttinReranker.rerank","models",
      {{"query",true,"string"},{"candidates",true,"array"}},true,false,false,"array"});
    out.push_back({"model.minilm-l6-v2.similarity","MiniLM-L6-v2.similarity","models",
      {{"left",true,"string"},{"right",true,"string"}},true,false,false,"number"});
    return out;
  }();
  return defs;
}
inline const api_definition* gutts_api_by_id(std::string_view id) {
  const auto& defs=gutts_api_definitions();
  const auto it=std::find_if(defs.begin(),defs.end(),[&](const api_definition& x){return x.id==id;});
  return it==defs.end()?nullptr:&*it;
}
inline const api_definition* find_portable_api(std::string_view id) {
  const auto& defs=portable_api_definitions();
  const auto it=std::find_if(defs.begin(),defs.end(),[&](const api_definition& x){return x.id==id;});
  return it==defs.end()?nullptr:&*it;
}
namespace api_detail {
inline const json_value& required(const json_value::object& input,std::string_view key) {
  const auto it=input.find(std::string(key));
  if(it==input.end())throw std::invalid_argument("missing required argument: "+std::string(key));
  return it->second;
}
inline const json_value* optional(const json_value::object& input,std::string_view key) noexcept {
  const auto it=input.find(std::string(key));return it==input.end()?nullptr:&it->second;
}
inline const std::string& str(const json_value& value) {
  if(const auto p=std::get_if<std::string>(&value.data))return *p;
  throw std::invalid_argument("expected string argument");
}
inline double num(const json_value& value) {
  if(const auto p=std::get_if<double>(&value.data))return *p;
  throw std::invalid_argument("expected numeric argument");
}
inline std::ptrdiff_t non_negative_int(const json_value& value) {
  const auto n=num(value);
  if(!std::isfinite(n)||n<0||std::trunc(n)!=n ||n>static_cast<double>(std::numeric_limits<std::ptrdiff_t>::max()))
    throw std::invalid_argument("expected non-negative integer argument");
  return static_cast<std::ptrdiff_t>(n);
}
inline const json_value::array& arr(const json_value& value) {
  if(const auto p=std::get_if<json_value::array>(&value.data))return *p;
  throw std::invalid_argument("expected array argument");
}
inline const json_value::object& obj(const json_value& value) {
  if(const auto p=std::get_if<json_value::object>(&value.data))return *p;
  throw std::invalid_argument("expected object argument");
}
inline std::vector<std::string> string_array(const json_value& value) {
  std::vector<std::string> out;
  for(const auto& v:arr(value))out.push_back(str(v));
  return out;
}
inline std::string argument_string(const json_value::object& in,const std::string& key) {return str(required(in,key));}
inline double argument_number(const json_value::object& in,const std::string& key) {return num(required(in,key));}
} // namespace api_detail
// A nullopt represents the JavaScript undefined result. Null is represented
// explicitly by json_value(nullptr). Invalid argument/unknown ID throws.
inline std::optional<json_value> execute_gutts_api(std::string_view api_id,const json_value::object& input) {
  using namespace api_detail;
  if(!find_portable_api(api_id)) {
    if(gutts_api_by_id(api_id))throw std::invalid_argument("GutTS API '"+std::string(api_id)+"' has metadata but no embedded runtime adapter.");
    throw std::invalid_argument("Unknown GutTS API '"+std::string(api_id)+"'.");
  }
  const auto last_dot=api_id.rfind('.');
  const auto name=api_id.substr(last_dot+1);
  if(name=="compact") {
    json_value::array out;
    // GutTS delegates to lodash compact: remove ALL falsy elements, not only null.
    // JSON cannot represent undefined or NaN, but false, zero and empty strings
    // are falsy; arrays and objects (including empty ones) remain truthy.
    for(const auto& x:arr(required(input,"values"))) {
      const bool keep=std::visit([](const auto& value)->bool {
        using V=std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<V,std::nullptr_t>)return false;
        else if constexpr(std::is_same_v<V,bool>)return value;
        else if constexpr(std::is_same_v<V,double>)return value!=0.0 && !std::isnan(value);
        else if constexpr(std::is_same_v<V,std::string>)return !value.empty();
        else return true;
      },x.data);
      if(keep)out.push_back(x);
    }
    return json_value(std::move(out));
  }
  if(name=="unique") {
    json_value::array out;
    // JavaScript SameValueZero compares object and array values by identity.
    // Distinct nested JSON objects are different references even when equal
    // structurally. Scalars retain SameValueZero equality, including +0/-0.
    for(const auto& x:arr(required(input,"values"))) {
      const bool seen=std::any_of(out.begin(),out.end(),[&](const json_value& existing) {
        if(std::holds_alternative<json_value::object>(x.data)||
           std::holds_alternative<json_value::array>(x.data)) return false;
        return existing.data==x.data;
      });
      if(!seen)out.push_back(x);
    }
    return json_value(std::move(out));
  }
  if(name=="chunk") {
    const auto& values=arr(required(input,"values"));const auto size=non_negative_int(required(input,"size"));
    if(!size)throw std::invalid_argument("size must be a positive integer");
    json_value::array out;
    const auto step=static_cast<std::size_t>(size);
    for(std::size_t begin=0;begin<values.size();) {
      const auto count=std::min(step,values.size()-begin);
      out.push_back(json_value(json_value::array(values.begin()+static_cast<std::ptrdiff_t>(begin),
        values.begin()+static_cast<std::ptrdiff_t>(begin+count))));begin+=count;
    }
    return json_value(std::move(out));
  }
  if(name=="flatten")return json_value(flatten_one(arr(required(input,"values"))));
  if(name=="last") {
    const auto& values=arr(required(input,"values"));
    return values.empty()?std::nullopt:std::optional<json_value>(values.back());
  }
  if(name=="isJsonValue")return json_value(is_json_value(required(input,"value")));
  if(name=="parseJson")return parse_json(argument_string(input,"text"));
  if(name=="tryParseJson")return try_parse_json(argument_string(input,"text"));
  if(name=="stringifyJson") {
    unsigned space=0;if(const auto* v=optional(input,"space"))space=static_cast<unsigned>(std::min<std::ptrdiff_t>(10,non_negative_int(*v)));
    return json_value(stringify_json(required(input,"value"),space));
  }
  if(name=="clamp")return json_value(clamp(argument_number(input,"value"),argument_number(input,"min"),argument_number(input,"max")));
  if(name=="inRange")return json_value(in_range(argument_number(input,"value"),argument_number(input,"min"),argument_number(input,"max")));
  if(name=="roundTo") {
    const auto* decimals=optional(input,"decimals");
    const auto places=decimals?non_negative_int(*decimals):0;
    if(places>std::numeric_limits<int>::max())throw std::invalid_argument("decimals out of range");
    return json_value(round_to(argument_number(input,"value"),static_cast<int>(places)));
  }
  if(name=="isFiniteNumber")return json_value(std::holds_alternative<double>(required(input,"value").data)
    &&is_finite_number(std::get<double>(required(input,"value").data)));
  if(name=="sum") {
    double total=0;for(const auto& v:arr(required(input,"values")))total+=num(v);
    return json_value(total);
  }
  if(name=="isPlainObject")return json_value(std::holds_alternative<json_value::object>(required(input,"value").data));
  if(name=="hasOwn") {
    const auto& o=obj(required(input,"value"));return json_value(o.find(argument_string(input,"key"))!=o.end());
  }
  if(name=="pick"||name=="omit") {
    const auto& o=obj(required(input,"value")); const auto keys=string_array(required(input,"keys"));
    json_value::object out;
    if(name=="pick") {
      for(const auto& key:keys){auto it=o.find(key);if(it!=o.end())out.emplace(*it);}
    }else {
      out=o;for(const auto& key:keys)out.erase(key);
    }
    return json_value(std::move(out));
  }
  if(name=="isBlank")return json_value(is_blank(argument_string(input,"value")));
  if(name=="isNonBlank")return json_value(is_non_blank(argument_string(input,"value")));
  if(name=="isEmail")return json_value(is_email(argument_string(input,"value")));
  if(name=="startsWithAny")return json_value(starts_with_any(argument_string(input,"value"),string_array(required(input,"prefixes"))));
  if(name=="endsWithAny")return json_value(ends_with_any(argument_string(input,"value"),string_array(required(input,"suffixes"))));
  if(name=="capitalize")return json_value(capitalize(argument_string(input,"value")));
  if(name=="camelCase")return json_value(camel_case(argument_string(input,"value")));
  if(name=="kebabCase")return json_value(kebab_case(argument_string(input,"value")));
  if(name=="snakeCase")return json_value(snake_case(argument_string(input,"value")));
  if(name=="truncate"||name=="truncateWords") {
    const auto value=argument_string(input,"value");
    const auto max=non_negative_int(required(input,name=="truncate"?"maxLength":"maxWords"));
    const auto* s=optional(input,"suffix");
    const auto suffix=s?str(*s):std::string("\xE2\x80\xA6");
    return json_value(name=="truncate"?truncate(value,max,suffix):truncate_words(value,max,suffix));
  }
  if(name=="isUrl")return json_value(is_url(argument_string(input,"value")));
  if(name=="joinUrl") {
    const auto base=argument_string(input,"base");
    const auto paths=string_array(required(input,"parts"));
    std::vector<std::string_view> refs;refs.reserve(paths.size());for(const auto& path:paths)refs.emplace_back(path);
    return json_value(ultrst::gut::join_url(base,refs));
  }
  if(name=="getQueryParam"){
    const auto value=get_query_param(argument_string(input,"url"),argument_string(input,"key"));
    return value?std::optional<json_value>(json_value(*value)):std::optional<json_value>(json_value(nullptr));
  }
  if(name=="setQueryParam")return json_value(set_query_param(argument_string(input,"url"),
    argument_string(input,"key"),argument_string(input,"value")));
  throw std::logic_error("Known GutTS API was not dispatched: "+std::string(api_id));
}
} // namespace gut
