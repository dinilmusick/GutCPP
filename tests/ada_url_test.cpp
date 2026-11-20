#include "gut/ada_url.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#define CHECK(x) do { if(!(x)){std::cerr<<"ada_url_test:"<<__LINE__<<": "<<#x<<'\n';return 1;} }while(false)
int main(){
 using namespace gut;
 CHECK(is_url_whatwg("https://example.com"));
 CHECK(is_url_whatwg("mailto:test@example.com"));
 CHECK(is_url_whatwg("data:text/plain,hello"));
 CHECK(!is_url_whatwg("relative/path"));
 CHECK(get_query_param_whatwg("https://example.com/a?x=hello+world&x=two","x")==std::optional<std::string>("hello world"));
 CHECK(!get_query_param_whatwg("https://example.com","missing"));
 CHECK(set_query_param_whatwg("https://EXAMPLE.com:443/x?a=1#frag","a","hello world") == "https://example.com/x?a=hello+world#frag");
 bool rejected=false;
 try {(void)set_query_param_whatwg("/relative","k","v");}catch(const std::invalid_argument&){rejected=true;}
 CHECK(rejected);
 return 0;
}
