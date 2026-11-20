#include "gut/storage_path.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do { if(!(x)) { std::cerr << "FAILED: " << #x << "\n"; std::exit(1); } } while(false)
static bool rejects(const std::string& s) { try { (void)gut::normalize_storage_path(s); } catch(const std::invalid_argument&) { return true; } return false; }
static bool rejects_encode(const std::string& s) { try { (void)gut::encode_safe_path_segments(s); } catch(const std::invalid_argument&) { return true; } return false; }
int main() {
 CHECK(gut::normalize_storage_path()=="");
 CHECK(gut::normalize_storage_path("///")=="");
 CHECK(gut::normalize_storage_path("//alpha///beta.txt/")=="alpha/beta.txt");
 CHECK(gut::normalize_storage_path("safe/%20name")=="safe/%20name");
 for(const auto& bad: {"../a","a/./b","a/%2e%2e/b","%252e%252e","%2f","%252f","%5c","%3f","%23","%00","%7f","%C0%AF","%ED%A0%80","%F4%90%80%80","bad%","bad%ZZ","a\\b","a?b","a#b","%2525252525252525252e"}) CHECK(rejects(bad));
 CHECK(gut::encode_safe_path_segments("//alpha///file name.txt/")=="alpha/file%20name.txt");
 CHECK(gut::encode_safe_path_segments("a/%20b")=="a/%2520b");
 CHECK(gut::encode_safe_path_segments("a/!~*'()_-.")=="a/!~*'()_-.");
 CHECK(gut::encode_safe_path_segments(u8"café/")=="caf%C3%A9");
 CHECK(rejects_encode("///"));
 CHECK(rejects_encode("../secret"));
 CHECK(rejects_encode("%252f"));
 // Golden vectors derived from GutTS normalizeStoragePath/encodeSafePathSegments semantics.
 CHECK(gut::normalize_storage_path("a//b///c") == "a/b/c");
 CHECK(gut::encode_safe_path_segments("folder/a+b&c=d.txt") == "folder/a%2Bb%26c%3Dd.txt");
 CHECK(gut::encode_safe_path_segments("folder/a@b:c.txt") == "folder/a%40b%3Ac.txt");
 CHECK(gut::encode_safe_path_segments("folder/100%25.txt") == "folder/100%2525.txt");
 CHECK(gut::encode_safe_path_segments(u8"emoji/😀.txt") == "emoji/%F0%9F%98%80.txt");
 for (const auto& bad: {"%E0%80%AF", "%F0%80%80%AF", "%ED%BF%BF", "%F4%90%80%80", "%25252e%25252e"}) CHECK(rejects(bad));

}
