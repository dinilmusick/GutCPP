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
  // GutTS does not normalize leading base slashes, internal repeated slashes,
  // query fragments or URL schemes; only trailing base and edge part slashes.
  assert(join_url("///alpha///"sv, {"//beta//"sv}) == "///alpha/beta");
  assert(join_url("http://example.test///"sv, {"////"sv, "//?q=1//"sv}) ==
         "http://example.test/?q=1");
  assert(join_url("////"sv, {"///"sv, ""sv}) == "");
  assert(join_url("prefix"sv, {"////a//b////"sv}) == "prefix/a//b");
  assert(join_url("a"sv, {"..//"sv, "../"sv}) == "a/../..");
  assert(join_url("a"sv, {"%2F"sv}) == "a/%2F");
  assert(join_url("a"sv, {"#fragment"sv}) == "a/#fragment");
  assert(join_url("a "sv, {" b "sv}) == "a / b ");
  assert(join_url("http://"sv, {"path"sv}) == "http:/path");
  assert(join_url("file:"sv, {"///etc/"sv}) == "file:/etc");

}
