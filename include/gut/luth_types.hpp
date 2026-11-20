#pragma once
// Portable, non-secret value contracts from pinned GutTS auth/luthAuthTypes.ts.
// These are data records, not proof of authentication or a network client.
#include "gut/portable.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gut {
enum class deployment_environment {production,staging,preview,development};
struct luth_deployment_record {
 std::string id;
 std::string url;
 std::optional<std::string> label;
 std::optional<deployment_environment> environment;
 bool is_active = false;
 std::optional<bool> is_primary;
 std::optional<int> priority;
};
class luth_auth_token_store;
struct luth_auth_client_config {
 std::string ecosystem_id;
 std::string application_id;
 std::optional<std::string> luth_url;
 std::vector<std::string> luth_urls;
 std::function<std::vector<std::string>()> luth_url_resolver;
 std::optional<std::string> origin;
 std::optional<deployment_environment> environment;
 std::optional<bool> auto_register;
 std::optional<int> max_retries;
 std::optional<int> retry_delay_ms;
 std::optional<int> timeout_ms;
 std::shared_ptr<luth_auth_token_store> token_store;
};
struct register_application_result {
 bool registered = false;
 std::string ecosystem_id;
 std::string application_id;
 std::string origin;
 deployment_environment environment=deployment_environment::development;
 bool authentication_enabled=false;
};
struct luth_user {
 std::string id;
 std::string name;
 std::string email;
 std::optional<std::string> image;
 std::optional<bool> email_verified;
};
struct luth_session {
 std::string id;
 std::string user_id;
 std::string expires_at;
 std::optional<std::string> token;
};
struct luth_session_result {
 bool authenticated=false;
 std::optional<luth_user> user;
 std::optional<luth_session> session;
 std::optional<std::string> error;
};
struct email_sign_in_credentials {std::string email;std::string password;};
struct email_sign_up_data {std::string email;std::string password;std::string name;};
struct google_sign_in_options {std::optional<std::string> callback_url;};
class luth_auth_error final:public std::runtime_error {
 std::string code_;
 std::optional<int> status_;
 public:
 luth_auth_error(std::string code,std::string message,std::optional<int> status=std::nullopt)
 :std::runtime_error("["+code+"] "+message),code_(std::move(code)),status_(status){}
 const std::string& code()const noexcept{return code_;}
 std::optional<int> status()const noexcept{return status_;}
};
inline bool validate_luth_auth_client_config(const luth_auth_client_config& config) noexcept {
 if(!valid_identity_id("application",config.application_id)||config.ecosystem_id.empty())return false;
 if((config.max_retries&&*config.max_retries<0) || (config.retry_delay_ms&&*config.retry_delay_ms<0) || (config.timeout_ms&&*config.timeout_ms<0))return false;
 for(const auto& url:config.luth_urls)if(url.empty())return false;
 return true;
}
// Token storage is an application-owned interface: stored secrets never enter
// global mutable state in this portable core.
class luth_auth_token_store {
 public:
 virtual ~luth_auth_token_store()=default;
 virtual std::string get() const=0;
 virtual void set(std::string token)=0;
 virtual void clear()=0;
};
} // namespace gut
