#include "gutcpp/url.hpp"
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

using ultrst::gut::join_url;
using namespace std::literals;

int main() {
  assert(join_url("https://example.com/"sv, {"/api/"sv, "v1"sv}) ==
         "https://example.com/api/v1");
  assert(join_url("///"sv, {"//"sv, "/a/"sv, ""sv, "b/"sv}) == "a/b");
  assert(join_url(""sv, {}) == "");
  assert(join_url("a///"sv, {}) == "a");
  assert(join_url("a"sv, {""sv, "///"sv}) == "a");
  assert(join_url("https://host"sv, {"?q=1"sv}) == "https://host/?q=1");
  assert(join_url("a"sv, {"x//y"sv}) == "a/x//y");
}
