#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace ultrst::gut {

// Mirrors GutTS joinUrl: strip trailing '/' from base, strip leading and
// trailing '/' from each part, discard empty normalized components.
inline std::string join_url(std::string_view base,
                            const std::vector<std::string_view>& parts) {
  const auto last = base.find_last_not_of('/');
  std::string result = last == std::string_view::npos
      ? std::string{} : std::string(base.substr(0, last + 1));
  for (auto part : parts) {
    const auto first = part.find_first_not_of('/');
    if (first == std::string_view::npos) continue;
    const auto end = part.find_last_not_of('/');
    part = part.substr(first, end - first + 1);
    if (!result.empty()) result.push_back('/');
    result.append(part.data(), part.size());
  }
  return result;
}

} // namespace ultrst::gut
