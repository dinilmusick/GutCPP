#pragma once
// Isolated native environment adapter for pinned GutTS src/server/auth.ts.
// The portable core never reads process environment implicitly.
#include <cstdlib>
#include <string>
#include <string_view>
namespace gut {
inline std::string resolve_server_runtime_app_url(){
 const char* value=std::getenv("APP_URL");
 if(!value)return {};
 std::string_view view(value);
 const auto start=view.find_first_not_of(" \t\r\n\f\v");
 if(start==std::string_view::npos)return {};
 const auto end=view.find_last_not_of(" \t\r\n\f\v");
 return std::string(view.substr(start,end-start+1));
}
} // namespace gut
