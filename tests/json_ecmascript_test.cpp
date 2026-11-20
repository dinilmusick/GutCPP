#include "gut/json.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"json_ecmascript_test "<<__LINE__<<" "<<#x<<'\n';std::exit(1);}}while(false)
int main(){
 using gut::json_value;using gut::stringify_json;using gut::parse_json;
 const std::vector<std::pair<double,std::string>> numbers={
   {0.1,"0.1"},{-0.,"0"},{1e-6,"0.000001"},{1e-7,"1e-7"},
   {1e20,"100000000000000000000"},{1e21,"1e+21"},{1.23e21,"1.23e+21"},
   {1000.0,"1000"},{0.000011,"0.000011"},{1234.5,"1234.5"},
   {std::numeric_limits<double>::denorm_min(),"5e-324"},
   {std::numeric_limits<double>::max(),"1.7976931348623157e+308"},
   {9007199254740991.0,"9007199254740991"},
   {1.0000000000000002,"1.0000000000000002"},
   {0.000000123456789,"1.23456789e-7"},
 };
 for(auto [value,expected]:numbers)CHECK(stringify_json(json_value(value))==expected);
 CHECK(stringify_json(json_value(std::numeric_limits<double>::infinity()))=="null");
 CHECK(stringify_json(json_value(-std::numeric_limits<double>::infinity()))=="null");
 CHECK(stringify_json(json_value(std::numeric_limits<double>::quiet_NaN()))=="null");
 CHECK(!gut::is_json_value(json_value(std::numeric_limits<double>::infinity())));
 CHECK(stringify_json(json_value(json_value::array{json_value(std::numeric_limits<double>::infinity()),json_value(.1),json_value(-0.)}))=="[null,0.1,0]");
 CHECK(std::isinf(parse_json("1e400").as<double>()));
 CHECK(std::isinf(parse_json("-1e400").as<double>())&&std::signbit(parse_json("-1e400").as<double>()));
 CHECK(parse_json("1e-999").as<double>()==0.);
 CHECK(stringify_json(parse_json("0.10000000000000001"))=="0.1");
 try{(void)stringify_json(json_value("\xC0\xAF"));CHECK(false);}catch(const std::invalid_argument&){}
}
