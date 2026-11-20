#pragma once
// Optional WHATWG URL API backed by pinned Ada URL C++ (the parser used by
// Node.js). No Node.js or browser dependency. Include only when Ada is linked.
#include <ada.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace gut {
inline bool is_url_whatwg(std::string_view input) {
  return static_cast<bool>(ada::parse<ada::url_aggregator>(input));
}
inline std::optional<std::string> get_query_param_whatwg(
    std::string_view text, std::string_view key) {
  auto parsed=ada::parse<ada::url_aggregator>(text);
  if(!parsed)throw std::invalid_argument("Invalid absolute URL");
  auto query=parsed->get_search();
  if(!query.empty()&&query.front()=='?')query.remove_prefix(1);
  ada::url_search_params params(query);
  const auto value=params.get(key);
  if(!value)return std::nullopt;
  return std::string(*value);
}
inline std::string set_query_param_whatwg(std::string_view text,
    std::string_view key,std::string_view value) {
  auto parsed=ada::parse<ada::url_aggregator>(text);
  if(!parsed)throw std::invalid_argument("Invalid absolute URL");
  auto query=parsed->get_search();
  if(!query.empty()&&query.front()=='?')query.remove_prefix(1);
  ada::url_search_params params(query);
  params.set(key,value);
  parsed->set_search(params.to_string());
  return std::string(parsed->get_href());
}
} // namespace gut
