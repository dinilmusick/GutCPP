#include "gut/engineering.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#define CHECK(x) do {if(!(x)){std::cerr<<"engineering_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
int main(){
 using namespace gut::engineering;
 CHECK(parse_provider_kind("github")==provider_kind::github);
 CHECK(parse_provider_kind("remote-sandbox")==provider_kind::remote_sandbox);
 CHECK(!parse_provider_kind("unknown"));
 CHECK(parse_capability("deployment.write")==capability::deployment_write);
 CHECK(!parse_capability("deployment.admin"));
 repository_ref repo{"repo-1",provider_kind::github,std::string("dinilmusick"),"GutCPP",std::string("main"),std::nullopt};
 CHECK(valid(repo));CHECK(!valid(repository_ref{"",provider_kind::git,std::nullopt,"repo",std::nullopt,std::nullopt}));
 workspace_ref workspace{"ws-1","/workspace",{repo},std::nullopt};CHECK(valid(workspace));
 execution_request request{"ws-1","cmake --build build",std::nullopt,{},1000.0,network_access::none,std::nullopt};
 CHECK(valid(request));
 request.timeout_ms=-1;CHECK(!valid(request));request.timeout_ms=1000;
 request.resources=execution_resources{2.0,512.0,1024.0};CHECK(valid(request));
 request.resources->memory_mb=std::numeric_limits<double>::infinity();CHECK(!valid(request));
 operation_context ctx{"task","trace",std::string("ws-1"),{capability_grant{capability::repository_read,std::string("repo-1"),std::nullopt}}};
 CHECK(has_grant(ctx,capability::repository_read,"repo-1"));
 CHECK(!has_grant(ctx,capability::repository_write,"repo-1"));
 CHECK(!has_grant(ctx,capability::repository_read,"repo-2"));
 ctx.grants.emplace_back(capability_grant{capability::verification_run,std::nullopt,std::nullopt});
 CHECK(has_grant(ctx,capability::verification_run));
}
