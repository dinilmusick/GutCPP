#include "gut/storage_path.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void check(bool ok, const char* context) {
 if (!ok) { std::cerr << "FAILED: " << context << '\n'; std::exit(1); }
}
static bool rejected(const std::string& input) {
 try { (void)gut::normalize_storage_path(input); }
 catch (const std::invalid_argument&) { return true; }
 return false;
}
int main() {
 // The GutTS contract normalizes separators without changing safe segments.
 const std::vector<std::string> safe = {
  "a", "alpha/beta", "with space/file.txt", "a+b&c=d", "a@b:c",
  "a!~*'()_-.", u8"café/😀", "a/%20b", "a/%2520b"
 };
 for (const auto& path : safe) {
  const auto normalized = gut::normalize_storage_path(path);
  check(gut::normalize_storage_path(normalized) == normalized, "normalization idempotence");
  check(gut::normalize_storage_path("///" + path + "//") == normalized, "separator normalization");
  check(!gut::encode_safe_path_segments(path).empty(), "safe path encoding");
 }
 // Every ASCII control byte and the DEL byte must be rejected.
 for (unsigned value = 0; value <= 31; ++value)
  check(rejected("a" + std::string(1, static_cast<char>(value)) + "b"), "control byte");
 check(rejected("a\x7fb"), "DEL byte");
 // Each percent decoding layer must be checked, including nested slash and dot.
 for (const auto& path : {"%2E", "%2e%2E", "%252e%252e", "%25252e%25252e",
                           "%2F", "%252f", "%25252F", "%255c", "%253f",
                           "%2523", "%2500", "%257f", "%25ZZ", "%"})
  check(rejected(path), "encoded traversal or delimiter");
 // Invalid UTF-8: stray continuation, truncated sequence and surrogate.
 for (const auto& path : {"%80", "%C3", "%E2%82", "%ED%A0%80", "%F4%90%80%80"})
  check(rejected(path), "invalid UTF-8");
 check(gut::encode_safe_path_segments(u8"café/😀") ==
       "caf%C3%A9/%F0%9F%98%80", "UTF-8 URI component encoding");
}
