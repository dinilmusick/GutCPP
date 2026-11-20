#include "gut/portable.hpp"
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
  return encoded == "a%2Fb" && gut::percent_encode_component("a/b") == "a%2Fb"
      && gut::normalize_storage_path("//safe///folder/") == "safe/folder"
      && ultrst::gut::join_url("https://example.com",{"/v1/"}) == "https://example.com/v1"
      && result.ok() && result.value() == 42 && gut::stringify_json(data) == R"({"safe":true})"
      && code.has_value() && gut::validate_oauth_redirect("https://auth.example.com/callback", "web").ok ? 0 : 1;
}
