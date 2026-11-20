#include "gut/api.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"ada_api_test line "<<__LINE__<<" "<<#x<<'\n';return 1;}}while(false)
int main(){
#ifndef GUTCPP_WITH_ADA_URL
#error "The opt-in gutcpp::ada_url target must define GUTCPP_WITH_ADA_URL"
#endif
  using gut::execute_gutts_api;
  CHECK(gut::find_portable_api("gutts.urls.setQueryParam")!=nullptr);
  const auto mail=execute_gutts_api("gutts.urls.isUrl",{{"value","mailto:hello@example.com"}});
  CHECK(mail.has_value() && mail->as<bool>());
  const auto invalid=execute_gutts_api("gutts.urls.isUrl",{{"value","relative/path"}});
  CHECK(invalid.has_value() && !invalid->as<bool>());
  const auto query=execute_gutts_api("gutts.urls.getQueryParam",{
    {"url","https://example.com/a?x=hello+world&x=second"},{"key","x"}
  });
  CHECK(query.has_value()&&query->as<std::string>()=="hello world");
  const auto missing=execute_gutts_api("gutts.urls.getQueryParam",{
    {"url","https://example.com"},{"key","missing"}
  });
  CHECK(missing.has_value()&&std::holds_alternative<std::nullptr_t>(missing->data));
  const auto replaced=execute_gutts_api("gutts.urls.setQueryParam",{
    {"url","https://EXAMPLE.com:443/x?a=1#frag"},{"key","a"},{"value","hello world"}
  });
  CHECK(replaced.has_value()&&replaced->as<std::string>()=="https://example.com/x?a=hello+world#frag");
  bool rejected=false;
  try{(void)execute_gutts_api("gutts.urls.getQueryParam",{{"url","/relative"},{"key","k"}});}
  catch(const std::invalid_argument&){rejected=true;}
  CHECK(rejected);
}
