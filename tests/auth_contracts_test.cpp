#include "gut/auth_contracts.hpp"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if(!(x)) {std::cerr<<"CHECK failed: "<<#x<<" at "<<__LINE__<<"\n";std::exit(1);} } while(false)
using namespace gut;
int main(){
  CHECK(contract_id("id_1"));CHECK(!contract_id("bad id"));
  CHECK(contract_app_id("app:prod"));CHECK(!contract_app_id("https://host"));
  auto registration=parse_json(R"({"schemaVersion":1,"appId":"app-1","clientId":"oauth-1","platform":"web","publicClient":true,"pkceMethod":"S256","redirectUris":["https://auth.example.com/callback"]})");
  CHECK(validate_oauth_client_registration(registration).ok);
  registration.as<json_value::object>()["pkceMethod"]="plain";
  CHECK(validate_oauth_client_registration(registration).reason=="INVALID_CLIENT_IDENTITY_OR_PKCE");
  CHECK(!validate_oauth_redirect("http://example.com/callback","web").ok);
  CHECK(validate_oauth_redirect("http://127.0.0.1:10808/callback","desktop").ok);
  CHECK(!validate_oauth_redirect("http://127.0.0.1/callback","desktop").ok);
  CHECK(!validate_oauth_redirect("https://example.com/callback","ios").ok);
  CHECK(validate_oauth_redirect("https://example.com/callback","ios",{"example.com"}).ok);
  auto capability=parse_json(R"({"schemaVersion":1,"appId":"target","operationId":"op-1","revision":"rev-1","displayName":"Example","requiredScopes":["scope.read"],"inputParameters":[{"name":"query","kind":"string","required":true}],"outputParameters":[],"kind":"backend","deploymentId":"deploy-1"})");
  CHECK(validate_capability_descriptor(capability).ok);
  capability.as<json_value::object>()["accessToken"]="secret";
  CHECK(validate_capability_descriptor(capability).reason=="INVALID_STATIC_CAPABILITY");
  capability.as<json_value::object>().erase("accessToken");
  auto invocation=parse_json(R"({"schemaVersion":1,"callerAppId":"caller","targetAppId":"target","audienceAppId":"target","principalUserId":"user-1","callerWorkspaceId":"workspace-1","targetWorkspaceId":"workspace-2","operationId":"op-1","expectedRevision":"rev-1","operationKind":"backend","grantId":"grant-1","traceId":"trace-1","activeAppChain":["caller"],"deadlineAtMs":100000,"maxDepth":4,"remainingCalls":5,"remainingRetries":2})");
  CHECK(validate_scoped_invocation(invocation).ok);
  CHECK(check_invocation_budget(invocation,100).ok);
  CHECK(validate_invocation_against_descriptor(invocation,capability,100).ok);
  const auto advanced=advance_invocation_budget(invocation,100);
  CHECK(advanced && advanced->remaining_calls==4 && advanced->remaining_retries==2);
  CHECK(advanced->active_app_chain==std::vector<std::string>({"caller","target"}));
  const auto retried=consume_invocation_retry(invocation,100);
  CHECK(retried && retried->remaining_calls==5 && retried->remaining_retries==1);
  CHECK(check_invocation_budget(invocation,100000).reason=="INVOCATION_DEADLINE_EXCEEDED");
  invocation.as<json_value::object>()["audienceAppId"]="someone-else";
  CHECK(validate_scoped_invocation(invocation).reason=="INVALID_INVOCATION_IDENTITY_OR_AUDIENCE");
  invocation.as<json_value::object>()["audienceAppId"]="target";
  invocation.as<json_value::object>()["activeAppChain"]=json_value::array{"target","caller"};
  CHECK(check_invocation_budget(invocation,100).reason=="INVOCATION_CYCLE");
  invocation.as<json_value::object>()["activeAppChain"]=json_value::array{"caller"};
  invocation.as<json_value::object>()["operationKind"]="frontend";
  CHECK(validate_scoped_invocation(invocation).reason=="FRONTEND_REQUIRES_LIVE_SESSION_AND_CONSENT");
}
