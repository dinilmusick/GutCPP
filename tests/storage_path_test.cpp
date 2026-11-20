#include "gut/storage_path.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do { if(!(x)) { std::cerr << "FAILED: " << #x << "\n"; std::exit(1); } } while(false)
static bool rejects(const std::string& s) { try { (void)gut::normalize_storage_path(s); } catch(const std::invalid_argument&) { return true; } return false; }
int main() {
 CHECK(gut::normalize_storage_path()=="");
 CHECK(gut::normalize_storage_path("///")=="");
 CHECK(gut::normalize_storage_path("//alpha///beta.txt/")=="alpha/beta.txt");
 CHECK(gut::normalize_storage_path("safe/%20name")=="safe/%20name");
 for(const auto& bad: {"../a","a/./b","a/%2e%2e/b","%252e%252e","%2f","%252f","%5c","%3f","%23","%00","%7f","%C0%AF","%ED%A0%80","%F4%90%80%80","bad%","bad%ZZ","a\\b","a?b","a#b","%2525252525252525252e"}) CHECK(rejects(bad));
}
