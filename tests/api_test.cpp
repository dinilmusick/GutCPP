#include "gut/api.hpp"
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"api_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
using namespace gut;
static std::optional<json_value> call(const std::string& id,const std::string& input){return execute_gutts_api("gutts."+id,parse_json(input).as<json_value::object>());}
int main(){
 const auto& registry=portable_api_definitions();
 CHECK(registry.size()==33);
 std::set<std::string> ids;for(const auto& definition:registry) {
   CHECK(ids.insert(definition.id).second);
   CHECK(find_portable_api(definition.id)!=nullptr);
   const auto node=gutts_api_to_fod_node(definition);
   CHECK(node.api_id==definition.id);
   CHECK(node.inputs.size()==definition.parameters.size());
   CHECK(node.type=="gutts/"+definition.id);
   CHECK(node.outputs.size()==1);
 }
 CHECK(call("arrays.compact",R"({"values":[1,null,2,null]})")==parse_json("[1,2]"));
 CHECK(call("arrays.compact",R"({"values":[null,0,false,"",[],{},1,null,2]})")==parse_json("[[],{},1,2]"));
 CHECK(call("arrays.unique",R"({"values":[1,2,1,3]})")==parse_json("[1,2,3]"));
 CHECK(call("arrays.chunk",R"({"values":[1,2,3,4,5],"size":2})")==parse_json("[[1,2],[3,4],[5]]"));
 CHECK(call("arrays.flatten",R"({"values":[1,[2,3],4]})")==parse_json("[1,2,3,4]"));
 CHECK(!call("arrays.last",R"({"values":[]})"));
 CHECK(call("arrays.last",R"({"values":[2,3]})")==json_value(3));
 CHECK(call("json.isJsonValue",R"({"value":{"a":true}})")==json_value(true));
 CHECK(call("json.parseJson",R"({"text":"[1,true]"})")==parse_json("[1,true]"));
 CHECK(!call("json.tryParseJson",R"({"text":"oops"})"));
 CHECK(call("json.stringifyJson",R"({"value":{"a":true}})")==json_value(R"({"a":true})"));
 CHECK(call("numbers.clamp",R"({"value":8,"min":1,"max":3})")==json_value(3));
 CHECK(call("numbers.inRange",R"({"value":3,"min":1,"max":3})")==json_value(false));
 CHECK(call("numbers.roundTo",R"({"value":1.235,"decimals":2})")==json_value(1.24));
 CHECK(call("numbers.isFiniteNumber",R"({"value":"nan"})")==json_value(false));
 CHECK(call("numbers.sum",R"({"values":[1,2,3]})")==json_value(6));
 CHECK(call("objects.hasOwn",R"({"value":{"x":1},"key":"x"})")==json_value(true));
 CHECK(call("objects.pick",R"({"value":{"x":1,"y":2},"keys":["x"]})")==parse_json(R"({"x":1})"));
 CHECK(call("objects.omit",R"({"value":{"x":1,"y":2},"keys":["x"]})")==parse_json(R"({"y":2})"));
 CHECK(call("objects.isPlainObject",R"({"value":[]})")==json_value(false));
 CHECK(call("strings.isBlank",R"({"value":"  "})")==json_value(true));
 CHECK(call("strings.isNonBlank",R"({"value":"a"})")==json_value(true));
 CHECK(call("strings.isEmail",R"({"value":"alice@example.com"})")==json_value(true));
 CHECK(call("strings.startsWithAny",R"({"value":"alpha","prefixes":["b","al"]})")==json_value(true));
 CHECK(call("strings.endsWithAny",R"({"value":"alpha","suffixes":["ha","x"]})")==json_value(true));
 CHECK(call("strings.capitalize",R"({"value":"FRED"})")==json_value("Fred"));
 CHECK(call("strings.camelCase",R"({"value":"HELLO there"})")==json_value("helloThere"));
 CHECK(call("strings.kebabCase",R"({"value":"HelloThere"})")==json_value("hello-there"));
 CHECK(call("strings.snakeCase",R"({"value":"HelloThere"})")==json_value("hello_there"));
 CHECK(call("strings.truncate",R"({"value":"abcde","maxLength":4,"suffix":"."})")==json_value("abc."));
 CHECK(call("strings.truncateWords",R"({"value":"one two three","maxWords":2,"suffix":"."})")==json_value("one two."));
 CHECK(call("urls.isUrl",R"({"value":"https://example.org"})")==json_value(true));
 CHECK(call("urls.joinUrl",R"({"base":"https://example.org/","parts":["/a/","b"]})")==json_value("https://example.org/a/b"));
 CHECK(call("urls.getQueryParam",R"({"url":"https://example.org/?x=a%20b","key":"x"})")==json_value("a b"));
 CHECK(call("urls.setQueryParam",R"({"url":"https://example.org/path","key":"x","value":"a b"})")==json_value("https://example.org/path?x=a+b"));
 bool rejected=false;try{(void)call("arrays.chunk",R"({"values":[1],"size":0})");}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
 rejected=false;try{(void)call("strings.truncate",R"({"value":"ab","maxLength":-1})");}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
 rejected=false;try{(void)call("numbers.sum",R"({"values":[2,"x"]})");}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
 rejected=false;try{(void)call("unknown.fake","{}");}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
}
