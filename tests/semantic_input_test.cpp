#include "gut/semantic_input.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"semantic_input_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
int main(){
 using namespace gut;
 auto number=resolve_deterministic_semantic_input("  -5.25  ",semantic_input_schema{"number",{}, {},{}});
 CHECK(number&&number->status=="resolved"&&number->value==json_value(-5.25));
 CHECK(!resolve_deterministic_semantic_input("oops",semantic_input_schema{"number",{}, {},{}}));
 CHECK(!resolve_deterministic_semantic_input("3.1",semantic_input_schema{"integer",{}, {},{}}));
 CHECK(resolve_deterministic_semantic_input("3",semantic_input_schema{"integer",{}, {},{}})->value==json_value(3));
 CHECK(resolve_deterministic_semantic_input(" TRUE ",semantic_input_schema{"boolean",{}, {},{}})->value==json_value(true));
 CHECK(!resolve_deterministic_semantic_input("yes",semantic_input_schema{"boolean",{}, {},{}}));
 CHECK(resolve_deterministic_semantic_input("a@b.co",semantic_input_schema{"string",std::string("email"),{}, {}}));
 CHECK(!resolve_deterministic_semantic_input("invalid",semantic_input_schema{"string",std::string("email"),{}, {}}));
 CHECK(resolve_deterministic_semantic_input("2026-11-20",semantic_input_schema{"string",std::string("date"),{}, {}}));
 CHECK(!resolve_deterministic_semantic_input("2026-11-20x",semantic_input_schema{"string",std::string("date"),{}, {}}));
 CHECK(resolve_deterministic_semantic_input("abc-42",semantic_input_schema{"string",{},std::string("^[a-z]+-[0-9]+$"), {}}));
 CHECK(!resolve_deterministic_semantic_input("https://example.com",semantic_input_schema{"string",{}, {},{}}));
 CHECK(resolve_deterministic_semantic_input("option-a",semantic_input_schema{"string",{}, {},{json_value("option-a"),json_value("option-b")}}));
 CHECK(!resolve_deterministic_semantic_input(" \t\n",semantic_input_schema{"number",{},{},{}}));
 CHECK(resolve_deterministic_semantic_input("\xE2\x80\x83 12 \xE2\x80\x83",semantic_input_schema{"number",{},{},{}})->value==json_value(12));
 semantic_input_cache cache(2);
 const semantic_input_resolution a{"resolved",json_value(1),"1",false};
 cache.set("one",a);cache.set("two",a);CHECK(cache.size()==2);
 CHECK(cache.get("one").has_value());cache.set("three",a);
 CHECK(!cache.get("two"));CHECK(cache.get("one"));CHECK(cache.get("three"));
 cache.clear();CHECK(cache.size()==0);
 semantic_input_cache disabled(0);disabled.set("a",a);CHECK(disabled.size()==0);
}
