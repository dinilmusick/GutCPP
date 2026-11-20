#include "gut/portable.hpp"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if(!(x)) { std::cerr << "CHECK failed: " << #x << " at " << __LINE__ << "\n"; std::exit(1); } } while(false)
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>
using namespace gut;
static bool invalid_utf8(std::string s){try{(void)utf8_scalars(s);return false;}catch(const std::invalid_argument&){return true;}}
int main(){
 CHECK((compact<int>({1,std::nullopt,2,std::nullopt,3})==std::vector<int>{1,2,3}));
 CHECK((unique<int>({1,2,1,3,2})==std::vector<int>{1,2,3}));
 CHECK((chunk<int>({1,2,3,4,5},2)==std::vector<std::vector<int>>{{1,2},{3,4},{5}}));
 try{(void)chunk<int>({1,2},0);CHECK(false);}catch(const std::invalid_argument&){}
 CHECK((flatten<int>({{1,2},{3},{4,5}})==std::vector<int>{1,2,3,4,5}));
 CHECK(!last(std::vector<int>{}).has_value());CHECK(last(std::vector<int>{1,2})==2);
 std::map<std::string,int> obj{{"a",1},{"b",2},{"c",3}};
 CHECK(has_own(obj,"a")&&!has_own(obj,"z"));
 CHECK((pick(obj,{"a","z"})==std::map<std::string,int>{{"a",1}}));
 CHECK((omit(obj,{"a","c"})==std::map<std::string,int>{{"b",2}}));
 CHECK(clamp(7,1,5)==5); CHECK(in_range(1,1,5));CHECK(!in_range(5,1,5));
 try{(void)clamp(1,3,2);CHECK(false);}catch(const std::invalid_argument&){}
 try{(void)in_range(1,3,2);CHECK(false);}catch(const std::invalid_argument&){}
 CHECK(round_to(1.234,2)==1.23);CHECK(round_to(-1.236,2)==-1.24);CHECK(round_to(-1.5)==-1);
 try{(void)round_to(1,-1);CHECK(false);}catch(const std::invalid_argument&){}
 CHECK(is_finite_number(1.0)); CHECK(!is_finite_number(std::numeric_limits<double>::infinity()));
 CHECK(sum({1,2,3})==6.0);CHECK(sum({})==0.0);
 CHECK(is_blank(" \t\n"));CHECK(is_blank("\xE2\x80\x83"));CHECK(is_blank("\xC2\xA0"));
 CHECK(is_non_blank(" caf\xC3\xA9 "));
 CHECK(starts_with_any("alpha",{"","beta"})); CHECK(ends_with_any("alpha",{"pha"}));
 CHECK(truncate_words("  one   two three  ",2,"...")=="one two...");
 CHECK(truncate_words("one two",3)=="one two");CHECK(truncate_words("one two",0)=="\xE2\x80\xA6");CHECK(truncate_words("one\xE2\x80\x83two three",1)=="one\xE2\x80\xA6");
 CHECK(invalid_utf8("\xC0\xAF"));CHECK(invalid_utf8("\xED\xA0\x80"));
 CHECK(invalid_utf8("\xF4\x90\x80\x80"));CHECK(invalid_utf8("\xE2\x82"));
 CHECK(utf8_scalars("\xF0\x9F\x98\x80").at(0)==0x1F600);
 CHECK(is_identity_kind("oauth_client"));CHECK(!is_identity_kind("browser"));
 CHECK(valid_identity_id("application","app_1:prod"));CHECK(!valid_identity_id("application","https://a"));
 CHECK(!valid_identity_id("application"," bad"));CHECK(!valid_identity_id("user",""));
 CHECK(parse_identity_ref("user","user-1").has_value());
 CHECK(!parse_identity_ref("user","a/b").has_value());
 CHECK(parse_legacy_application_id({{"app_id","app-1"},{"appId","app-1"}})=="app-1");
 CHECK(!parse_legacy_application_id({{"app_id","app-1"},{"appId","app-2"}}));
}
