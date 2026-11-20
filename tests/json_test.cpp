#include "gut/json.hpp"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if(!(x)) { std::cerr << "CHECK failed: " << #x << " at " << __LINE__ << "\n"; std::exit(1); } } while(false)
#include <cmath>
#include <limits>
#include <string>
using namespace gut;
static bool rejects(std::string_view s){try{(void)parse_json(s);return false;}catch(const std::invalid_argument&){return true;}}
int main(){
 CHECK(parse_json("null")==json_value(nullptr));
 CHECK(parse_json("true")==json_value(true));CHECK(parse_json("false")==json_value(false));
 CHECK(parse_json("-1.25e2")==json_value(-125));
 CHECK(parse_json("\"\\uD83D\\uDE00\"")==json_value("\xF0\x9F\x98\x80"));
 CHECK(parse_json("\"\\u00E9\"")==json_value("\xC3\xA9"));
 CHECK(parse_json("{\"z\":1,\"a\":[true,null,\"x\"]}")
   ==json_value((json_value::object{{"a",json_value::array{true,nullptr,"x"}},{"z",1}})));
 CHECK(stringify_json(parse_json("[null,true,0,-1,\"x\"]"))=="[null,true,0,-1,\"x\"]");
 CHECK(stringify_json(json_value("hello\n\"\\"))=="\"hello\\n\\\"\\\\\"");
 for(auto invalid:{"", "-", "+1", "01", "1.", "1e", "[1,]", "{\"a\":1,}", "\"\\uD800\"", "\"\\uDC00\"", "\"\\x00\"", "true false", "[false true]", "{\"k\" 1}"})CHECK(rejects(invalid));
 CHECK(std::isinf(parse_json("1e9999").as<double>()));
 CHECK(!try_parse_json("[1,]").has_value());
 CHECK(try_parse_json("null").has_value());
 CHECK(!is_json_value(json_value(std::numeric_limits<double>::infinity())));
 CHECK(!is_json_value(json_value("\xC0\xAF")));
 CHECK(rejects("\"\xC0\xAF\""));
 CHECK(stringify_json(parse_json("{\"a\":\"b\"}"),2)=="{\n  \"a\": \"b\"\n}");
}
