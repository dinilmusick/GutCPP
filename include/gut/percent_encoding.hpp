#pragma once
#include <string>
#include <string_view>

namespace gut {
// Encode a single URI component as RFC 3986 percent-encoded bytes.
// UTF-8 is preserved as individually encoded bytes; no locale dependencies.
inline std::string percent_encode_component(std::string_view input) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string output;
  output.reserve(input.size());
  for (unsigned char byte : input) {
    const bool unreserved = (byte >= 'A' && byte <= 'Z') ||
      (byte >= 'a' && byte <= 'z') || (byte >= '0' && byte <= '9') ||
      byte == '-' || byte == '.' || byte == '_' || byte == '~';
    if (unreserved) output.push_back(static_cast<char>(byte));
    else {
      output.push_back('%');
      output.push_back(hex[byte >> 4]);
      output.push_back(hex[byte & 0x0F]);
    }
  }
  return output;
}
} // namespace gut
