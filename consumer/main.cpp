#include "gut/portable.hpp"
#include "gut/strings.hpp"
#include "gut/api.hpp"
#include "gut/engineering.hpp"
#include "gut/model_residency.hpp"
#include "gut/model_library.hpp"
#include "gut/server_auth.hpp"
#include "gut/model_registry.hpp"
#include "gut/semantic_input.hpp"
#include "gut/semantic_workflow.hpp"
#include "gut/ettin_head.hpp"
#include "gut/minilm_math.hpp"
#include "gut/luth_types.hpp"
#include "gut/luth_client.hpp"
#include "gut/google_drive.hpp"
#include "gut/application_auth_client.hpp"
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
  const auto metadata=gut::gutts_model_by_id("model.cactus-needle3");
  const auto semantic=gut::resolve_deterministic_semantic_input("true",gut::semantic_input_schema{"boolean",{}, {},{}});
  gut::semantic_workflow_request native_semantic_request;
  native_semantic_request.input="true";
  native_semantic_request.schema.type="boolean";
  const auto native_semantic=gut::resolve_semantic_workflow(native_semantic_request);
  const auto native_ettin=gut::compute_ettin_head_scores(
    gut::ettin_hidden_states{1,1,1,{0.5f}},
    gut::ettin_head_weights{{1.0f},{1.0f},{0.0f},{1.0f},{0.0f}});
  gut::luth_auth_client_config auth;auth.ecosystem_id="ultrst";auth.application_id="app-1";
  return native_ettin.size()==1 && native_ettin[0]>0 && native_ettin[0]<1 && native_semantic.status=="resolved" && native_semantic.value && *native_semantic.value==gut::json_value(true) && gut::valid_local_auth_path("/api/auth/oidc") && gut::luth_detail::environment_for_origin("https://preview.example.com") == gut::deployment_environment::preview && metadata && metadata->name=="Needle 3" && semantic && semantic->value==gut::json_value(true) &&
    gut::validate_luth_auth_client_config(auth) && api && api->as<std::string>() == "helloWorld" && model == 42 &&
      provider && *provider == gut::engineering::provider_kind::github && encoded == "a%2Fb" && gut::percent_encode_component("a/b") == "a%2Fb"
      && gut::normalize_storage_path("//safe///folder/") == "safe/folder"
      && ultrst::gut::join_url("https://example.com",{"/v1/"}) == "https://example.com/v1"
      && result.ok() && result.value() == 42 && gut::stringify_json(data) == R"({"safe":true})"
      && code.has_value() && gut::validate_oauth_redirect("https://auth.example.com/callback", "web").ok ? 0 : 1;
}
