#pragma once
// Native JSON-object path traversal for the pinned lodash has/pick/omit APIs.
// This header operates on value-owned JSON; no prototypes or JavaScript heap.
#include "gut/json.hpp"
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
namespace object_path_detail {
inline constexpr std::size_t max_depth=64, max_index=4095;
inline std::vector<std::string> tokenize(std::string_view path) {
  if(path.size()>4096)throw std::invalid_argument("object path too long");
  if(path.empty())return {""};
  std::vector<std::string> result;
  std::string current;
  bool after_bracket=false;
  for(std::size_t i=0;i<path.size();) {
    if(path[i]=='.') {
      if(!after_bracket)result.push_back(std::move(current));
      else after_bracket=false;
      current.clear();++i;continue;
    }
    if(path[i]=='[') {
      if(!current.empty())result.push_back(std::move(current));
      current.clear();++i;
      if(i>=path.size())throw std::invalid_argument("unterminated object path bracket");
      const char quote=(path[i]=='\''||path[i]=='"')?path[i++]:0;
      bool finished=false;
      for(;i<path.size();++i) {
        if(quote) {
          if(path[i]=='\\'&&i+1<path.size()) {current+=path[++i];continue;}
          if(path[i]==quote) {++i;finished=true;break;}
        } else if(path[i]==']') {finished=true;break;}
        current+=path[i];
      }
      if(!finished||i>=path.size()||path[i]!=']')throw std::invalid_argument("unterminated object path bracket");
      ++i;result.push_back(std::move(current));current.clear();after_bracket=true;continue;
    }
    current+=path[i++];after_bracket=false;
  }
  if(!current.empty()||path.back()=='.')result.push_back(std::move(current));
  if(result.empty())result.emplace_back("");
  if(result.size()>max_depth)throw std::invalid_argument("object path exceeds maximum depth");
  return result;
}
inline std::vector<std::string> path_tokens(const json_value::object& root,std::string_view path) {
  // lodash chooses an actual top-level key before parsing a dotted path.
  if(root.find(std::string(path))!=root.end())return {std::string(path)};
  return tokenize(path);
}
inline bool index_of(std::string_view token,std::size_t& index) noexcept {
  if(token.empty()||(token.size()>1&&token.front()=='0'))return false;
  std::size_t value=0;
  for(char c:token) {
    if(c<'0'||c>'9')return false;
    if(value>(max_index-static_cast<std::size_t>(c-'0'))/10)return false;
    value=value*10+static_cast<std::size_t>(c-'0');
  }
  index=value;return true;
}
inline const json_value* get(const json_value& root,const std::vector<std::string>& tokens) {
  const json_value* current=&root;
  for(const auto& token:tokens) {
    if(const auto obj=std::get_if<json_value::object>(&current->data)) {
      const auto it=obj->find(token);if(it==obj->end())return nullptr;current=&it->second;
    }else if(const auto arr=std::get_if<json_value::array>(&current->data)) {
      std::size_t index=0;if(!index_of(token,index)||index>=arr->size())return nullptr;
      current=&(*arr)[index];
    }else return nullptr;
  }
  return current;
}
inline bool dangerous(std::string_view token)noexcept {
  // Never synthesize JavaScript prototype-like paths on behalf of a consumer.
  return token=="__proto__"||token=="prototype"||token=="constructor";
}
inline void set(json_value& root,const std::vector<std::string>& tokens,const json_value& value) {
  if(tokens.empty())return;
  json_value* current=&root;
  for(std::size_t i=0;i<tokens.size();++i) {
    if(dangerous(tokens[i]))return;
    const bool last=i+1==tokens.size();
    if(auto obj=std::get_if<json_value::object>(&current->data)) {
      if(last){obj->insert_or_assign(tokens[i],value);return;}
      const bool numeric_next=[&]{std::size_t n=0;return index_of(tokens[i+1],n);}();
      auto [it,created]=obj->try_emplace(tokens[i]);
      if(created || std::holds_alternative<std::nullptr_t>(it->second.data)) {
        it->second=numeric_next?json_value(json_value::array{}):json_value(json_value::object{});
      }
      current=&it->second;
    } else if(auto arr=std::get_if<json_value::array>(&current->data)) {
      std::size_t index=0;
      if(!index_of(tokens[i],index))return;
      if(index>=arr->size())arr->resize(index+1);
      if(last){(*arr)[index]=value;return;}
      if(std::holds_alternative<std::nullptr_t>((*arr)[index].data)){
        std::size_t next=0;
        (*arr)[index]=index_of(tokens[i+1],next)?json_value(json_value::array{}):json_value(json_value::object{});
      }
      current=&(*arr)[index];
    }else return;
  }
}
inline void erase(json_value& root,const std::vector<std::string>& tokens) {
  if(tokens.empty())return;
  for(const auto& token:tokens)if(dangerous(token))return;
  json_value* current=&root;
  for(std::size_t i=0;i<tokens.size();++i) {
    const bool last=i+1==tokens.size();
    if(auto obj=std::get_if<json_value::object>(&current->data)) {
      auto it=obj->find(tokens[i]);if(it==obj->end())return;
      if(last){obj->erase(it);return;}current=&it->second;
    }else if(auto arr=std::get_if<json_value::array>(&current->data)) {
      std::size_t index=0;if(!index_of(tokens[i],index)||index>=arr->size())return;
      if(last){(*arr)[index]=json_value(nullptr);return;}current=&(*arr)[index];
    }else return;
  }
}
} // namespace object_path_detail
inline bool json_has_path(const json_value::object& obj,std::string_view path) {
  const json_value root(obj);
  return object_path_detail::get(root,object_path_detail::path_tokens(obj,path))!=nullptr;
}
inline json_value::object json_pick_paths(const json_value::object& obj,const std::vector<std::string>& paths) {
  const json_value root(obj);
  json_value output(json_value::object{});
  for(const auto& path:paths){
    const auto tokens=object_path_detail::path_tokens(obj,path);
    if(const auto* found=object_path_detail::get(root,tokens))object_path_detail::set(output,tokens,*found);
  }
  return std::move(std::get<json_value::object>(output.data));
}
inline json_value::object json_omit_paths(const json_value::object& obj,const std::vector<std::string>& paths) {
  json_value output(obj);
  for(const auto& path:paths)object_path_detail::erase(output,object_path_detail::path_tokens(obj,path));
  return std::move(std::get<json_value::object>(output.data));
}
} // namespace gut
