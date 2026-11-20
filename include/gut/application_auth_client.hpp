#pragma once
// Native counterpart of GutTS auth/applicationAuthClient.ts at the pinned commit.
// The host supplies a same-origin, cookie-aware HTTP transport. This class never
// reads or forwards bearer tokens, OAuth codes, PKCE secrets or cookies.
#include "gut/json.hpp"
#include "gut/luth_types.hpp"
#include "gut/net.hpp"
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace gut {
struct application_auth_request {
  std::string method;
  std::string path; // Only same-origin relative paths are ever generated.
  std::string body;
  std::map<std::string, std::string> headers;
  bool same_origin_credentials = true; // Cookie jar is owned by the host.
  bool no_store = true;
};
struct application_auth_response {
  int status = 0;
  std::string body;
  bool ok() const noexcept { return status >= 200 && status < 300; }
};
using application_auth_fetcher = std::function<application_auth_response(const application_auth_request&)>;
using application_auth_navigator = std::function<void(const std::string&)>;

inline bool valid_local_auth_path(std::string_view path) noexcept {
  if(path.empty() || path.front() != '/' || path.find("//") != std::string_view::npos) return false;
  for(unsigned char c : path) {
    const bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      (c >= '0' && c <= '9') || c == '/' || c == '_' || c == '-';
    if(!valid) return false;
  }
  return true;
}
inline bool valid_auth_return_path(std::string_view path) noexcept {
  if(path.empty() || path.front() != '/' ||
     (path.size() > 1 && path[1] == '/')) return false;
  for(unsigned char c : path) {
    if(c == '\\' || c <= 0x1F || c == 0x7F) return false;
  }
  return true;
}

class application_auth_client {
  std::string base_path_;
  application_auth_fetcher fetch_;
  application_auth_navigator navigate_;

  application_auth_response request(application_auth_request req,
                                    std::string_view failure_code,
                                    std::string_view failure_message) const {
    if(!fetch_) throw luth_auth_error(std::string(failure_code),std::string(failure_message),503);
    try { return fetch_(req); }
    catch(const std::exception&) {
      throw luth_auth_error(std::string(failure_code),std::string(failure_message),503);
    }
    catch(...) {
      throw luth_auth_error(std::string(failure_code),std::string(failure_message),503);
    }
  }

  static const json_value::object& as_object(const json_value& value) {
    if(const auto* obj=std::get_if<json_value::object>(&value.data)) return *obj;
    throw luth_auth_error("AUTH_SESSION_INVALID","Application returned an invalid authenticated session.");
  }
  static std::optional<std::string> string_at(const json_value::object& o,std::string_view key) {
    const auto it=o.find(std::string(key));
    if(it==o.end())return std::nullopt;
    if(const auto* value=std::get_if<std::string>(&it->second.data))return *value;
    return std::nullopt;
  }
  static std::optional<bool> bool_at(const json_value::object& o,std::string_view key) {
    const auto it=o.find(std::string(key));
    if(it==o.end()) return std::nullopt;
    if(const auto* value=std::get_if<bool>(&it->second.data))return *value;
    return std::nullopt;
  }
  static std::optional<json_value::object> object_at(const json_value::object& o,std::string_view key) {
    const auto it=o.find(std::string(key));
    if(it==o.end())return std::nullopt;
    if(const auto* value=std::get_if<json_value::object>(&it->second.data))return *value;
    return std::nullopt;
  }

  static luth_session_result parse_session(const std::string& response) {
    try {
      const auto root=as_object(parse_json(response));
      // As in GutTS, all 2xx replies must contain an authenticated user and session.
      const auto authenticated=bool_at(root,"authenticated");
      if(authenticated != std::optional<bool>{true})
        throw luth_auth_error("AUTH_SESSION_INVALID","Application returned an invalid authenticated session.");
      const auto user=object_at(root,"user");
      const auto session=object_at(root,"session");
      if(!user || !session)
        throw luth_auth_error("AUTH_SESSION_INVALID","Application returned an invalid authenticated session.");
      const auto uid=string_at(*user,"id"),email=string_at(*user,"email"),
                 sid=string_at(*session,"id");
      if(!uid || uid->empty() || !email || email->empty() || !sid || sid->empty())
        throw luth_auth_error("AUTH_SESSION_INVALID","Application returned an invalid authenticated session.");
      luth_session_result out;
      out.authenticated=true;
      out.user=luth_user{*uid,string_at(*user,"name").value_or(""),*email,
                         string_at(*user,"image"),bool_at(*user,"emailVerified")};
      out.session=luth_session{*sid,string_at(*session,"userId").value_or(*uid),
                                string_at(*session,"expiresAt").value_or(""),std::nullopt};
      // Intentionally discard session.token: the BFF session belongs only in HttpOnly cookies.
      return out;
    } catch(const luth_auth_error&) { throw; }
    catch(const std::exception&) {
      throw luth_auth_error("AUTH_SESSION_INVALID","Application returned an invalid authenticated session.");
    }
  }

public:
  explicit application_auth_client(application_auth_fetcher fetcher,
                                   application_auth_navigator navigator = {},
                                   std::string base_path="/api/auth/oidc")
    :base_path_(std::move(base_path)),fetch_(std::move(fetcher)),navigate_(std::move(navigator)) {
    if(!valid_local_auth_path(base_path_))
      throw luth_auth_error("INVALID_APPLICATION_AUTH_PATH","App authentication must use a fixed local path.");
    if(base_path_.size()>1 && base_path_.back()=='/')base_path_.pop_back();
  }
  const std::string& base_path() const noexcept{return base_path_;}
  std::map<std::string,std::string> get_auth_headers() const{return {};}
  void start_sign_in(std::string return_to="/") const {
    if(!valid_auth_return_path(return_to))
      throw luth_auth_error("INVALID_AUTH_RETURN_PATH","Login return path must be local.");
    if(!navigate_)throw luth_auth_error("NAVIGATION_UNAVAILABLE","Native host must supply an application navigator.");
    navigate_(base_path_+"/start?next="+encode_uri_component(return_to));
  }
  void sign_in_with_google(std::string return_to="/") const{start_sign_in(std::move(return_to));}
  void sign_in_with_email(std::string return_to="/") const{start_sign_in(std::move(return_to));}
  luth_session_result get_session() const {
    const auto response=request({"GET",base_path_+"/session",{}, {},true,true},
      "AUTH_SESSION_UNAVAILABLE","Application authentication service is unavailable.");
    if(response.status==401)return {false,std::nullopt,std::nullopt,std::string("SESSION_INVALID")};
    if(!response.ok())throw luth_auth_error("AUTH_SESSION_UNAVAILABLE",
      "Application authentication service is unavailable.",response.status);
    return parse_session(response.body);
  }
  void sign_out() const {
    application_auth_request req{"POST",base_path_+"/logout","{}",
      {{"content-type","application/json"}},true,true};
    const auto response=request(std::move(req),"AUTH_LOGOUT_UNAVAILABLE","Logout service is unavailable.");
    if(!response.ok())throw luth_auth_error("AUTH_LOGOUT_REJECTED","Logout was rejected by the application.",response.status);
    if(get_session().authenticated)
      throw luth_auth_error("AUTH_LOGOUT_UNVERIFIED","Application session remains valid after logout.");
  }
};
} // namespace gut