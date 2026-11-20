#pragma once
// Production Google Drive v3 REST client. Supplied OAuth token is used only on
// the pinned Google API origin. A deliberately opt-in loopback origin supports
// deterministic integration tests without forwarding real credentials.
// This adapter performs actual HTTP requests through the injected transport.
#include "gut/json.hpp"
#include "gut/luth_client.hpp" // Shared native HTTP request/response transport.
#include "gut/percent_encoding.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
class google_drive_error : public std::runtime_error {
  int status_;
public:
  explicit google_drive_error(const std::string& message, int status = 0)
      : std::runtime_error(message), status_(status) {}
  int status() const noexcept { return status_; }
};

struct google_drive_options {
  // Do not override for production. Custom URL only permitted with explicit
  // allow_loopback_test_origin and a strictly verified loopback HTTP authority.
  std::string origin = "https://www.googleapis.com";
  bool allow_loopback_test_origin = false;
  int timeout_ms = 30000;
  std::size_t max_response_bytes = 64u * 1024u * 1024u;
};

class google_drive_client {
  luth_http_transport transport_;
  std::function<std::string()> token_provider_;
  std::string origin_;
  int timeout_ms_;
  std::size_t max_response_bytes_;

  static std::string component(std::string_view text) {
    // encodeURIComponent preserves !'()*; percent_encode_component follows
    // strict RFC3986. Both are accepted in a URL query or path component.
    return percent_encode_component(text);
  }
  static void require_file_id(std::string_view id) {
    if(id.empty() || id.size()>256) throw std::invalid_argument("Invalid Drive file ID");
    for(const unsigned char ch:id) {
      if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
           (ch>='0'&&ch<='9')||ch=='-'||ch=='_'))
        throw std::invalid_argument("Invalid Drive file ID");
    }
  }
  static void require_no_controls(std::string_view text, std::string_view field) {
    if(text.empty() || text.size()>4096) throw std::invalid_argument(std::string("Invalid ")+std::string(field));
    for(unsigned char c:text) if(c<32 || c==127)
      throw std::invalid_argument(std::string("Invalid ")+std::string(field));
    (void)utf8_scalars(text); // Validate UTF-8 before putting it into JSON.
  }
  static std::string query(const std::vector<std::pair<std::string,std::string>>& parameters) {
    std::string result;
    for(const auto& [name,value]:parameters) {
      if(!result.empty())result+='&';
      result+=component(name); result+='='; result+=component(value);
    }
    return result.empty()?"":"?"+result;
  }
  luth_http_response request(const std::string& method,
                             const std::string& path,
                             std::string body = {},
                             std::string content_type = {}) const {
    if(path.empty() || path.front()!='/' || (path.size()>1&&path[1]=='/') ||
       path.find('\0')!=std::string::npos || path.find('\\')!=std::string::npos ||
       path.find('#')!=std::string::npos)
      throw std::invalid_argument("Invalid Drive API path");
    const std::string token=token_provider_();
    if(token.empty() || token.size()>16384 || token.find_first_of("\r\n")!=std::string::npos ||
       token.find('\0')!=std::string::npos)
      throw google_drive_error("Google Drive OAuth access token unavailable");
    luth_http_request req;
    req.method=method;
    req.url=origin_+path;
    req.body=std::move(body);
    req.timeout_ms=timeout_ms_;
    req.no_store=true;
    req.credentials_include=false; // Bearer auth only; never send browser cookies.
    req.headers.emplace("Authorization","Bearer "+token);
    req.headers.emplace("Accept","application/json");
    if(!content_type.empty())req.headers.emplace("Content-Type",std::move(content_type));
    const auto response=transport_(req);
    if(response.body.size()>max_response_bytes_)
      throw google_drive_error("Google Drive response exceeds configured limit",response.status);
    if(response.status<200 || response.status>=300)
      throw google_drive_error("Google Drive HTTP request failed",response.status);
    return response;
  }
  static json_value::object object_response(const luth_http_response& response) {
    const auto parsed=try_parse_json(response.body);
    if(!parsed || !std::holds_alternative<json_value::object>(parsed->data))
      throw google_drive_error("Invalid Google Drive JSON object",response.status);
    return parsed->as<json_value::object>();
  }
  static std::string path_for_id(std::string_view id) {
    require_file_id(id);return "/drive/v3/files/"+component(id);
  }
public:
  google_drive_client(luth_http_transport transport,
                      std::function<std::string()> oauth_access_token,
                      google_drive_options options = {})
      : transport_(std::move(transport)), token_provider_(std::move(oauth_access_token)),
        origin_(std::move(options.origin)), timeout_ms_(options.timeout_ms),
        max_response_bytes_(options.max_response_bytes) {
    if(!transport_ || !token_provider_ || timeout_ms_<=0 || timeout_ms_>600000 ||
       max_response_bytes_==0 || max_response_bytes_>512u*1024u*1024u)
      throw std::invalid_argument("Invalid Google Drive client configuration");
    if(origin_!="https://www.googleapis.com") {
      const auto parsed=net_detail::parse_absolute_url(origin_);
      if(!options.allow_loopback_test_origin || parsed.scheme!="http" ||
         !luth_detail::loopback_authority(parsed.authority) ||
         parsed.path!="/" || parsed.query_present || parsed.fragment_present ||
         parsed.origin()!=origin_)
        throw std::invalid_argument("Google Drive endpoint must be the official HTTPS API origin");
    }
  }

  json_value::object list_files(std::string filter="trashed = false",
                               std::string page_token={}, int page_size=100,
                               std::string fields="nextPageToken,files(id,name,mimeType,parents,size,modifiedTime)") const {
    if(page_size<1 || page_size>1000 || filter.size()>4096 ||
       page_token.size()>4096 || fields.empty() || fields.size()>4096)
      throw std::invalid_argument("Invalid Google Drive list parameters");
    std::vector<std::pair<std::string,std::string>> params={{"pageSize",std::to_string(page_size)},
                                                           {"fields",std::move(fields)}};
    if(!filter.empty())params.emplace_back("q",std::move(filter));
    if(!page_token.empty())params.emplace_back("pageToken",std::move(page_token));
    return object_response(request("GET","/drive/v3/files"+query(params)));
  }
  json_value::object get_metadata(std::string_view id,
                                   std::string fields="id,name,mimeType,parents,size,modifiedTime") const {
    if(fields.empty() || fields.size()>4096)throw std::invalid_argument("Invalid Google Drive fields");
    return object_response(request("GET",path_for_id(id)+query({{"fields",std::move(fields)}})));
  }
  std::string download_media(std::string_view id) const {
    return request("GET",path_for_id(id)+"?alt=media").body;
  }
  std::string export_file(std::string_view id,std::string mime_type) const {
    require_no_controls(mime_type,"export MIME type");
    return request("GET",path_for_id(id)+"/export"+query({{"mimeType",std::move(mime_type)}})).body;
  }
  json_value::object create_metadata(std::string name,std::string mime_type,
                                     std::vector<std::string> parent_ids = {}) const {
    require_no_controls(name,"file name"); require_no_controls(mime_type,"MIME type");
    json_value::object metadata{{"name",std::move(name)},{"mimeType",std::move(mime_type)}};
    if(!parent_ids.empty()) {
      json_value::array parents;
      for(auto& parent:parent_ids){require_file_id(parent);parents.emplace_back(std::move(parent));}
      metadata.emplace("parents",std::move(parents));
    }
    return object_response(request("POST","/drive/v3/files",stringify_json(json_value(std::move(metadata))),"application/json"));
  }
  json_value::object upload_media(const std::string& bytes,std::string mime_type) const {
    require_no_controls(mime_type,"MIME type");
    return object_response(request("POST","/upload/drive/v3/files?uploadType=media",bytes,std::move(mime_type)));
  }
  json_value::object update_media(std::string_view id,const std::string& bytes,std::string mime_type) const {
    require_no_controls(mime_type,"MIME type");
    return object_response(request("PATCH","/upload"+path_for_id(id)+"?uploadType=media",bytes,std::move(mime_type)));
  }
  json_value::object rename_file(std::string_view id,std::string name) const {
    require_no_controls(name,"file name");
    return object_response(request("PATCH",path_for_id(id),stringify_json(json_value(json_value::object{{"name",std::move(name)}})),"application/json"));
  }
  json_value::object copy_file(std::string_view id,std::string name) const {
    require_no_controls(name,"file name");
    return object_response(request("POST",path_for_id(id)+"/copy",stringify_json(json_value(json_value::object{{"name",std::move(name)}})),"application/json"));
  }
  void delete_file(std::string_view id) const {(void)request("DELETE",path_for_id(id));}
};
} // namespace gut
