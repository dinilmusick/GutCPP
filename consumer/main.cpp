#include "gut/portable.hpp"
#include "gut/strings.hpp"
#include "gut/api.hpp"
#include "gut/engineering.hpp"
#include "gut/model_residency.hpp"
#include "gut/json.hpp"
#include "gut/net.hpp"
#include "gut/auth_contracts.hpp"
#include "gut/result.hpp"
#include "gut/storage_path.hpp"
#include "gut/percent_encoding.hpp"
#include "gutcpp/url.hpp"
#include <string>
int main() {
  const auto encoded = gut::percent_encode_rfc3986_component("a/b");
  const auto data = gut::parse_json(R"({"safe":true})");
  const auto code = gut::parse_identity_ref("application", "app_1");
  const auto result = gut::Result<int>::success(42);
  const auto api = gut::execute_gutts_api("gutts.strings.camelCase",gut::json_value::object{{"value","Hello World"}});
  gut::model_residency<int> residency;
  const int model = residency.run([]{return 41;},[](int&){},[](int& x){return x+1;});
  const auto provider = gut::engineering::parse_provider_kind("github");
  return api && api->as<std::string>() == "helloWorld" && model == 42 &&
      provider && *provider == gut::engineering::provider_kind::github && encoded == "a%2Fb" && gut::percent_encode_component("a/b") == "a%2Fb"
      && gut::normalize_storage_path("//safe///folder/") == "safe/folder"
      && ultrst::gut::join_url("https://example.com",{"/v1/"}) == "https://example.com/v1"
      && result.ok() && result.value() == 42 && gut::stringify_json(data) == R"({"safe":true})"
      && code.has_value() && gut::validate_oauth_redirect("https://auth.example.com/callback", "web").ok ? 0 : 1;
}
