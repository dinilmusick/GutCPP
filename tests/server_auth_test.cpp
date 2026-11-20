#include "gut/server_auth.hpp"
#include <cstdlib>
#include <iostream>
int main(){
 if(gut::resolve_server_runtime_app_url()=="https://example.com/app")return 0;
 std::cerr<<"APP_URL adapter did not preserve and trim the configured server origin\n";
 return 1;
}
