#include "gut/portable.hpp"
#include "gut/json.hpp"
#include "gut/net.hpp"
#include "gut/auth_contracts.hpp"
#include <string>
int main() {
  const auto encoded = gut::percent_encode_rfc3986_component("a/b");
  const auto data = gut::parse_json(R"({"safe":true})");
  const auto code = gut::parse_identity_ref("application", "app_1");
  return encoded == "a%2Fb" && gut::stringify_json(data) == R"({"safe":true})"
      && code.has_value() && gut::validate_oauth_redirect("https://auth.example.com/callback", "web").ok ? 0 : 1;
}
