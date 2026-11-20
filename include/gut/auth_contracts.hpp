#pragma once
// Non-authoritative, fail-closed shape and budget validation for ULTRST v1 wire contracts.
// No function in this header authenticates callers, verifies grants, or approves access.
#include "gut/json.hpp"
#include "gut/net.hpp"
#include <set>

namespace gut {
struct contract_check {
  bool ok;
  std::string reason;
  static contract_check pass(){return {true,{}};}
  static contract_check fail(std::string why){return {false,std::move(why)};}
};
namespace contract_detail {
inline const json_value::object* record(const json_value& j) noexcept {
  return std::get_if<json_value::object>(&j.data);
}
inline const json_value* find(const json_value::object& obj,std::string_view key) noexcept {
  auto it=obj.find(std::string(key));return it==obj.end()?nullptr:&it->second;
}
inline const std::string* text(const json_value* v) noexcept {
  return v?std::get_if<std::string>(&v->data):nullptr;
}
inline const bool* boolean(const json_value* v) noexcept {
  return v?std::get_if<bool>(&v->data):nullptr;
}
inline const double* number(const json_value* v) noexcept {
  return v?std::get_if<double>(&v->data):nullptr;
}
inline const json_value::array* array(const json_value* v) noexcept {
  return v?std::get_if<json_value::array>(&v->data):nullptr;
}
inline bool exact_keys(const json_value::object& o,const std::set<std::string>& allowed) {
  return std::all_of(o.begin(),o.end(),[&](const auto& pair){return allowed.count(pair.first)!=0;});
}
inline bool id(const json_value* v) {
  auto s=text(v);if(!s||s->empty()||s->size()>256||s->front()==' '||s->back()==' ')return false;
  for(unsigned char c:*s)if(c=='\\'||c<=32||c==127)return false;
  return true;
}
inline bool app_id(const json_value* v) {
  const auto s=text(v);return s&&valid_identity_id("application",*s);
}
inline bool small_integer(const json_value* v,std::int64_t low,std::int64_t high) {
  const auto n=number(v);
  return n&&std::isfinite(*n)&&std::trunc(*n)==*n&&*n>=static_cast<double>(low)&&*n<=static_cast<double>(high);
}
inline bool unique_strings(const json_value* v,std::size_t max_size,bool identifiers=true) {
  const auto a=array(v);if(!a||a->size()>max_size)return false;
  std::set<std::string> keys;
  for(const auto& item:*a){
    auto s=text(&item);if(!s||(identifiers&&!id(&item))||!keys.insert(*s).second)return false;
  }
  return true;
}
inline bool capability_parameters(const json_value* v) {
  const auto a=array(v);if(!a||a->size()>128)return false;
  std::set<std::string> names;
  for(const auto& item:*a){
    const auto o=record(item);
    if(!o||!exact_keys(*o,{"name","kind","required"})||o->size()!=3)return false;
    auto kind=text(find(*o,"kind"));auto name=text(find(*o,"name"));
    if(!id(find(*o,"name"))||!kind||!name||!boolean(find(*o,"required")))return false;
    if(*kind!="string"&&*kind!="number"&&*kind!="integer"&&*kind!="boolean"
       &&*kind!="object"&&*kind!="array")return false;
    if(!names.insert(*name).second)return false;
  }
  return true;
}
inline bool host_valid(std::string_view host) {
  if(host.empty()||host.front()=='.'||host.find("..")!=std::string_view::npos)return false;
  for(char c:host)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='-'))return false;
  return true;
}
} // namespace contract_detail
inline bool contract_id(std::string_view value) {
  json_value owned{std::string(value)};
  return contract_detail::id(&owned);
}
inline bool contract_app_id(std::string_view value) noexcept {
  return valid_identity_id("application",value);
}
inline contract_check validate_oauth_redirect(std::string_view uri,std::string_view platform,
                                            const std::vector<std::string>& verified_hosts={}) {
  if(uri.size()>2048||uri.empty()||uri.front()==' '||uri.back()==' '
      ||uri.find('*')!=std::string_view::npos||uri.find('#')!=std::string_view::npos)
    return contract_check::fail("INVALID_REDIRECT_URI");
  net_detail::parsed_url url;
  try{url=net_detail::parse_absolute_url(uri);}catch(...){return contract_check::fail("INVALID_REDIRECT_URI");}
  if(url.fragment_present||url.query_present||url.path=="/"||url.authority.find('@')!=std::string::npos
     ||url.serialized()!=uri)return contract_check::fail("NON_CANONICAL_REDIRECT_URI");
  std::string host=url.authority;
  const auto port_marker=host.rfind(':');
  std::string port;
  if(port_marker!=std::string::npos){port=host.substr(port_marker+1);host.erase(port_marker);}
  if(platform=="desktop"){
    if(url.scheme!="http"||(host!="127.0.0.1"&&host!="[::1]")||port.empty())
      return contract_check::fail("DESKTOP_REQUIRES_EXACT_LOOPBACK_CALLBACK");
    std::uint32_t parsed_port=0;
    for(char c:port){if(c<'0'||c>'9')return contract_check::fail("DESKTOP_REQUIRES_EXACT_LOOPBACK_CALLBACK");
      parsed_port=parsed_port*10+static_cast<unsigned>(c-'0');if(parsed_port>65535)return contract_check::fail("DESKTOP_REQUIRES_EXACT_LOOPBACK_CALLBACK");}
    if(parsed_port==0)return contract_check::fail("DESKTOP_REQUIRES_EXACT_LOOPBACK_CALLBACK");
  } else {
    if(url.scheme!="https"||host.empty()||host=="localhost"||host.find(".localhost")!=std::string::npos
       ||host.find(':')!=std::string::npos||!contract_detail::host_valid(host))
      return contract_check::fail("WEB_OR_MOBILE_REQUIRES_HTTPS");
    if((platform=="ios"||platform=="android")
       &&std::find(verified_hosts.begin(),verified_hosts.end(),host)==verified_hosts.end())
      return contract_check::fail("MOBILE_HOST_NOT_VERIFIED");
  }
  return contract_check::pass();
}
inline contract_check validate_oauth_client_registration(const json_value& input) {
  using namespace contract_detail;
  const auto* o=record(input);
  if(!o||!exact_keys(*o,{"schemaVersion","appId","clientId","platform","publicClient","pkceMethod","redirectUris","verifiedHosts"}))
    return contract_check::fail("INVALID_CLIENT_FIELDS");
  const auto version=number(find(*o,"schemaVersion"));
  const auto platform=text(find(*o,"platform"));
  const auto pub=boolean(find(*o,"publicClient"));
  const auto pkce=text(find(*o,"pkceMethod"));
  if(!version||*version!=1||!app_id(find(*o,"appId"))||!id(find(*o,"clientId"))
     ||!platform||(*platform!="web"&&*platform!="ios"&&*platform!="android"&&*platform!="desktop")
     ||!pub||!*pub||!pkce||*pkce!="S256")
    return contract_check::fail("INVALID_CLIENT_IDENTITY_OR_PKCE");
  std::vector<std::string> hosts;
  if(const auto* host_field=find(*o,"verifiedHosts")){
    const auto values=array(host_field);
    if(!values)return contract_check::fail("INVALID_VERIFIED_HOSTS");
    for(const auto& v:*values){const auto host=text(&v);if(!host||!host_valid(*host))return contract_check::fail("INVALID_VERIFIED_HOSTS");hosts.push_back(*host);}
  }
  const auto redirects=array(find(*o,"redirectUris"));
  if(!redirects||redirects->empty()||redirects->size()>32||!unique_strings(find(*o,"redirectUris"),32,false))
    return contract_check::fail("INVALID_REDIRECT_SET");
  for(const auto& r:*redirects){const auto* uri=text(&r);
    if(!uri)return contract_check::fail("INVALID_REDIRECT_SET");
    auto c=validate_oauth_redirect(*uri,*platform,hosts);if(!c.ok)return c;
  }
  return contract_check::pass();
}
inline contract_check validate_capability_descriptor(const json_value& input) {
  using namespace contract_detail;
  const auto o=record(input);
  const auto bad_base=contract_check::fail("INVALID_CAPABILITY_BASE");
  if(!o)return bad_base;
  const auto version=number(find(*o,"schemaVersion"));
  const auto kind=text(find(*o,"kind"));
  const auto name=text(find(*o,"displayName"));
  if(!version||*version!=1||!kind||!app_id(find(*o,"appId"))
    ||!id(find(*o,"operationId"))||!id(find(*o,"revision"))
    ||!name||name->empty()||name->size()>160||is_blank(*name)
    ||!unique_strings(find(*o,"requiredScopes"),64)
    ||!capability_parameters(find(*o,"inputParameters"))
    ||!capability_parameters(find(*o,"outputParameters")))return bad_base;
  const std::set<std::string> base={"schemaVersion","appId","operationId","revision","displayName",
    "requiredScopes","inputParameters","outputParameters","kind"};
  auto allowed=base;
  if(*kind=="workflow"){
    allowed.insert({"workflowId","ownerUserId","sourceWorkspaceId","published"});
    if(!exact_keys(*o,allowed)||!id(find(*o,"workflowId"))||!id(find(*o,"ownerUserId"))
      ||!id(find(*o,"sourceWorkspaceId"))||!boolean(find(*o,"published"))
      ||!*boolean(find(*o,"published")))
      return contract_check::fail("INVALID_OR_PRIVATE_WORKFLOW_PUBLICATION");
  }else if(*kind=="backend"||*kind=="frontend"){
    allowed.insert({"deploymentId","sessionPermission"});
    const auto permission=text(find(*o,"sessionPermission"));
    if(!exact_keys(*o,allowed)||!id(find(*o,"deploymentId"))
      ||(*kind=="backend"&&permission)
      ||(*kind=="frontend"&&(!permission||(*permission!="inspect"&&*permission!="act"))))
      return contract_check::fail("INVALID_STATIC_CAPABILITY");
  }else return contract_check::fail("UNKNOWN_CAPABILITY_KIND");
  return contract_check::pass();
}
inline contract_check validate_scoped_invocation(const json_value& input) {
  using namespace contract_detail;
  const auto o=record(input);
  if(!o||!exact_keys(*o,{"schemaVersion","callerAppId","targetAppId","audienceAppId","principalUserId",
    "callerWorkspaceId","targetWorkspaceId","operationId","expectedRevision","operationKind",
    "frontendSessionId","frontendPermission","frontendConsentId","grantId","traceId","activeAppChain",
    "deadlineAtMs","maxDepth","remainingCalls","remainingRetries"}))
    return contract_check::fail("INVALID_INVOCATION_IDENTITY_OR_AUDIENCE");
  const auto version=number(find(*o,"schemaVersion"));
  const auto kind=text(find(*o,"operationKind"));
  const auto target=text(find(*o,"targetAppId")),audience=text(find(*o,"audienceAppId"));
  if(!version||*version!=1||!kind||(*kind!="backend"&&*kind!="frontend"&&*kind!="workflow")
    ||!app_id(find(*o,"callerAppId"))||!app_id(find(*o,"targetAppId"))
    ||!app_id(find(*o,"audienceAppId"))||!target||!audience||*target!=*audience
    ||!id(find(*o,"principalUserId"))||!id(find(*o,"callerWorkspaceId"))
    ||!id(find(*o,"targetWorkspaceId"))||!id(find(*o,"operationId"))
    ||!id(find(*o,"expectedRevision"))||!id(find(*o,"grantId"))||!id(find(*o,"traceId")))
    return contract_check::fail("INVALID_INVOCATION_IDENTITY_OR_AUDIENCE");
  const auto frontend_session=find(*o,"frontendSessionId");
  const auto frontend_consent=find(*o,"frontendConsentId");
  const auto permission=text(find(*o,"frontendPermission"));
  if(*kind=="frontend"){
    if(!id(frontend_session)||!id(frontend_consent)||!permission
       ||(*permission!="inspect"&&*permission!="act"))
      return contract_check::fail("FRONTEND_REQUIRES_LIVE_SESSION_AND_CONSENT");
  }else if(frontend_session||frontend_consent||find(*o,"frontendPermission"))
    return contract_check::fail("NON_FRONTEND_SESSION_NOT_ALLOWED");
  const auto chain=array(find(*o,"activeAppChain"));
  const auto caller=text(find(*o,"callerAppId"));
  if(!chain||chain->empty()||chain->size()>16||!caller)
    return contract_check::fail("INVALID_CALL_ANCESTRY");
  for(const auto& member:*chain)if(!app_id(&member))return contract_check::fail("INVALID_CALL_ANCESTRY");
  const auto tail=text(&chain->back());
  if(!tail||*tail!=*caller)return contract_check::fail("INVALID_CALL_ANCESTRY");
  if(!small_integer(find(*o,"deadlineAtMs"),-9007199254740991LL,9007199254740991LL)
     ||!small_integer(find(*o,"maxDepth"),1,16)
     ||!small_integer(find(*o,"remainingCalls"),0,1000)
     ||!small_integer(find(*o,"remainingRetries"),0,20))
    return contract_check::fail("INVALID_EXECUTION_BUDGET");
  return contract_check::pass();
}
inline contract_check check_invocation_budget(const json_value& input,double now_ms) {
  auto checked=validate_scoped_invocation(input);if(!checked.ok)return checked;
  using namespace contract_detail;
  const auto& o=*record(input);
  if(!std::isfinite(now_ms)||now_ms>=*number(find(o,"deadlineAtMs")))
    return contract_check::fail("INVOCATION_DEADLINE_EXCEEDED");
  const auto chain=array(find(o,"activeAppChain"));
  if(chain->size()>=static_cast<std::size_t>(*number(find(o,"maxDepth"))))
    return contract_check::fail("INVOCATION_MAX_DEPTH");
  const auto target=text(find(o,"targetAppId"));
  for(const auto& member:*chain)if(*text(&member)==*target)
    return contract_check::fail("INVOCATION_CYCLE");
  if(*number(find(o,"remainingCalls"))==0)
    return contract_check::fail("INVOCATION_WORK_BUDGET_EXHAUSTED");
  return contract_check::pass();
}
inline contract_check validate_invocation_against_descriptor(const json_value& invocation,
    const json_value& descriptor,double now_ms) {
  auto checked=check_invocation_budget(invocation,now_ms);if(!checked.ok)return checked;
  checked=validate_capability_descriptor(descriptor);if(!checked.ok)return checked;
  using namespace contract_detail;
  const auto& i=*record(invocation);const auto& d=*record(descriptor);
  if(*text(find(d,"appId"))!=*text(find(i,"targetAppId"))
    ||*text(find(d,"operationId"))!=*text(find(i,"operationId"))
    ||*text(find(d,"revision"))!=*text(find(i,"expectedRevision"))
    ||*text(find(d,"kind"))!=*text(find(i,"operationKind")))
    return contract_check::fail("TARGET_CAPABILITY_REVISION_OR_KIND_MISMATCH");
  if(*text(find(d,"kind"))=="workflow"
    &&*text(find(d,"sourceWorkspaceId"))!=*text(find(i,"targetWorkspaceId")))
    return contract_check::fail("WORKFLOW_WORKSPACE_MISMATCH");
  if(*text(find(d,"kind"))=="frontend"
    &&*text(find(d,"sessionPermission"))!=*text(find(i,"frontendPermission")))
    return contract_check::fail("FRONTEND_PERMISSION_MISMATCH");
  return contract_check::pass();
}
struct invocation_budget {
  std::vector<std::string> active_app_chain;
  std::uint64_t remaining_calls=0;
  std::uint64_t remaining_retries=0;
  std::int64_t deadline_at_ms=0;
};
inline std::optional<invocation_budget> advance_invocation_budget(const json_value& input,double now_ms) {
  if(!check_invocation_budget(input,now_ms).ok)return std::nullopt;
  using namespace contract_detail;const auto& o=*record(input);
  invocation_budget result;
  for(const auto& id:*array(find(o,"activeAppChain")))result.active_app_chain.push_back(*text(&id));
  result.active_app_chain.push_back(*text(find(o,"targetAppId")));
  result.remaining_calls=static_cast<std::uint64_t>(*number(find(o,"remainingCalls")))-1;
  result.remaining_retries=static_cast<std::uint64_t>(*number(find(o,"remainingRetries")));
  result.deadline_at_ms=static_cast<std::int64_t>(*number(find(o,"deadlineAtMs")));
  return result;
}
inline std::optional<invocation_budget> consume_invocation_retry(const json_value& input,double now_ms) {
  if(!check_invocation_budget(input,now_ms).ok)return std::nullopt;
  using namespace contract_detail;const auto& o=*record(input);
  const auto retries=static_cast<std::uint64_t>(*number(find(o,"remainingRetries")));
  if(retries==0)return std::nullopt;
  invocation_budget result;
  for(const auto& id:*array(find(o,"activeAppChain")))result.active_app_chain.push_back(*text(&id));
  result.remaining_calls=static_cast<std::uint64_t>(*number(find(o,"remainingCalls")));
  result.remaining_retries=retries-1;
  result.deadline_at_ms=static_cast<std::int64_t>(*number(find(o,"deadlineAtMs")));
  return result;
}
} // namespace gut
