#include "gut/luth_types.hpp"
#include <cstdio>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
struct local_tokens final : gut::luth_auth_token_store {
  std::string token;
  std::string get() const override { return token; }
  void set(std::string value) override { token = std::move(value); }
  void clear() override { token.clear(); }
};
void check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
void run_checks() {
  gut::luth_auth_client_config config;
  config.ecosystem_id = "ultrst";
  config.application_id = "app-1";
  check(gut::validate_luth_auth_client_config(config), "valid config rejected");
  config.max_retries = -1;
  check(!gut::validate_luth_auth_client_config(config), "negative retries permitted");
  config.max_retries = 3;
  config.application_id = "https://host";
  check(!gut::validate_luth_auth_client_config(config), "origin accepted as app ID");

  local_tokens store;
  gut::luth_auth_token_store& dynamic = store;
  dynamic.set("secret");
  check(dynamic.get() == "secret", "token store set failed");
  dynamic.clear();
  check(dynamic.get().empty(), "token store clear failed");

  gut::luth_session_result result;
  check(!result.authenticated && !result.user && !result.session,
        "new session result not empty");
  result.authenticated = true;
  result.user = gut::luth_user{"user-1", "Name", "a@example.org", std::nullopt, true};
  result.session = gut::luth_session{"session-1", "user-1", "2026-12-01", std::nullopt};
  check(result.user->id == result.session->user_id, "session user mismatch");

  bool caught = false;
  try {
    throw gut::luth_auth_error("NOT_AUTHENTICATED", "Missing token", 401);
  } catch (const gut::luth_auth_error& ex) {
    caught = true;
    check(ex.code() == "NOT_AUTHENTICATED", "auth error code mismatch");
    check(ex.status().has_value() && *ex.status() == 401, "auth error status mismatch");
    check(std::string(ex.what()) == "[NOT_AUTHENTICATED] Missing token",
          "auth error message mismatch");
  }
  check(caught, "auth error was not caught");
}
} // namespace

int main() {
  try {
    run_checks();
    return 0;
  } catch (const std::exception& ex) {
    std::fprintf(stderr, "luth_types_test failed: %s\n", ex.what());
    return 1;
  } catch (...) {
    std::fprintf(stderr, "luth_types_test failed: unknown exception\n");
    return 1;
  }
}