#pragma once
// Isolated native environment adapter for pinned GutTS src/server/auth.ts.
// The portable core never reads process environment implicitly.
#include <cstdlib>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

namespace gut {
inline std::string resolve_server_runtime_app_url(){
#if defined(_MSC_VER)
  // On Windows the secure CRT allocates an owning snapshot of the environment
  // value. Do not use the deprecated getenv API or leak the returned allocation.
  char* value=nullptr;
  std::size_t length=0;
  const auto status=_dupenv_s(&value,&length,"APP_URL");
  const std::unique_ptr<char,decltype(&std::free)> owner(value,&std::free);
  if(status!=0||value==nullptr)return {};
#else
  const char* value=std::getenv("APP_URL");
  if(value==nullptr)return {};
#endif
  std::string_view view(value);
  const auto start=view.find_first_not_of(" \t\r\n\f\v");
  if(start==std::string_view::npos)return {};
  const auto end=view.find_last_not_of(" \t\r\n\f\v");
  return std::string(view.substr(start,end-start+1));
}
} // namespace gut
