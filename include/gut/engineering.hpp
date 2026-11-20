#pragma once
// Pure, platform-independent equivalents of pinned GutTS engineering data contracts.
#include "gut/portable.hpp"
#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gut {
namespace engineering {
using identifier=std::string;
enum class provider_kind {github,gitlab,bitbucket,git,local,container,webcontainer,remote_sandbox,ci};
enum class network_access {none,restricted,full};
enum class execution_status {succeeded,failed,cancelled,timed_out};
enum class check_status {passed,failed,skipped,blocked};
enum class report_status {passed,failed,blocked};
enum class capability {
 repository_read, repository_write, git_write, workspace_create, execution_run,
 browser_run, verification_run, artifact_write, package_publish,
 deployment_write, promotion_write
};
inline std::string_view wire_name(provider_kind kind) noexcept {
 switch(kind){
 case provider_kind::github:return "github";case provider_kind::gitlab:return "gitlab";
 case provider_kind::bitbucket:return "bitbucket";case provider_kind::git:return "git";
 case provider_kind::local:return "local";case provider_kind::container:return "container";
 case provider_kind::webcontainer:return "webcontainer";case provider_kind::remote_sandbox:return "remote-sandbox";
 case provider_kind::ci:return "ci";
 }
 return {}; // unreachable for the listed enumerators; invalid casts rejected by decoder
}
inline std::optional<provider_kind> parse_provider_kind(std::string_view name) noexcept {
 for(auto value:{provider_kind::github,provider_kind::gitlab,provider_kind::bitbucket,
  provider_kind::git,provider_kind::local,provider_kind::container,
  provider_kind::webcontainer,provider_kind::remote_sandbox,provider_kind::ci})
   if(wire_name(value)==name)return value;
 return std::nullopt;
}
inline std::string_view wire_name(capability kind) noexcept {
 switch(kind){
 case capability::repository_read:return "repository.read";
 case capability::repository_write:return "repository.write";
 case capability::git_write:return "git.write";
 case capability::workspace_create:return "workspace.create";
 case capability::execution_run:return "execution.run";
 case capability::browser_run:return "browser.run";
 case capability::verification_run:return "verification.run";
 case capability::artifact_write:return "artifact.write";
 case capability::package_publish:return "package.publish";
 case capability::deployment_write:return "deployment.write";
 case capability::promotion_write:return "promotion.write";
 }
 return {};
}
inline std::optional<capability> parse_capability(std::string_view name) noexcept {
 for(auto value:{capability::repository_read,capability::repository_write,capability::git_write,
  capability::workspace_create,capability::execution_run,capability::browser_run,
  capability::verification_run,capability::artifact_write,capability::package_publish,
  capability::deployment_write,capability::promotion_write})
   if(wire_name(value)==name)return value;
 return std::nullopt;
}
struct repository_ref {
 identifier id;
 provider_kind provider=provider_kind::git;
 std::optional<std::string> owner;
 std::string name;
 std::optional<std::string> default_branch;
 std::optional<std::string> remote;
};
struct workspace_ref {
 identifier id;
 std::string root;
 std::vector<repository_ref> repositories;
 std::optional<identifier> snapshot_id;
};
struct execution_resources {
 std::optional<double> cpu;
 std::optional<double> memory_mb;
 std::optional<double> disk_mb;
};
struct execution_request {
 identifier workspace_id;
 std::string command;
 std::optional<std::string> cwd;
 std::map<std::string,std::string> environment;
 std::optional<double> timeout_ms;
 std::optional<network_access> network;
 std::optional<execution_resources> resources;
};
struct execution_result {
 identifier execution_id;
 execution_status status=execution_status::failed;
 std::optional<int> exit_code;
 std::string stdout_text;
 std::string stderr_text;
 std::vector<identifier> artifact_ids;
};
struct verification_check {
 identifier id;
 std::string name;
 check_status status=check_status::blocked;
 std::optional<double> duration_ms;
 std::optional<identifier> execution_id;
 std::optional<std::string> details;
};
struct verification_report {
 identifier id;
 report_status status=report_status::blocked;
 std::vector<verification_check> checks;
 std::vector<identifier> artifact_ids;
};
struct artifact_ref {
 identifier id;
 std::string digest;
 std::string type;
 std::optional<std::string> uri;
 std::optional<identifier> source_execution_id;
};
struct capability_grant {
 capability permission=capability::repository_read;
 std::optional<std::string> resource;
 std::optional<std::string> expires_at;
};
struct operation_context {
 identifier task_id;
 identifier trace_id;
 std::optional<identifier> workspace_id;
 std::vector<capability_grant> grants;
};
struct provenance_record {
 identifier operation_id;
 identifier actor_id;
 identifier task_id;
 identifier trace_id;
 std::string timestamp;
 std::optional<repository_ref> source;
 std::optional<std::string> commit;
 std::vector<identifier> artifacts;
};
inline bool valid_id(std::string_view id) noexcept {
 if(id.empty()||id.size()>256)return false;
 return std::all_of(id.begin(),id.end(),[](char c){return static_cast<unsigned char>(c)>=0x20&&c!='\x7f';});
}
inline bool valid(const repository_ref& repository) noexcept {
 return valid_id(repository.id)&&!repository.name.empty()&&parse_provider_kind(wire_name(repository.provider)).has_value();
}
inline bool valid(const workspace_ref& workspace) noexcept {
 if(!valid_id(workspace.id)||workspace.root.empty())return false;
 return std::all_of(workspace.repositories.begin(),workspace.repositories.end(),
   [](const repository_ref& repo){return valid(repo);});
}
inline bool valid(const execution_request& request) noexcept {
 if(!valid_id(request.workspace_id)||request.command.empty())return false;
 if(request.timeout_ms&&(!std::isfinite(*request.timeout_ms)||*request.timeout_ms<0))return false;
 if(request.resources){
  const auto good=[](const std::optional<double>& value){return !value || (std::isfinite(*value) && *value>=0);};
  if(!good(request.resources->cpu)||!good(request.resources->memory_mb)||!good(request.resources->disk_mb))return false;
 }
 return true;
}
inline bool has_grant(const operation_context& context,capability required,std::string_view resource={}) {
 return std::any_of(context.grants.begin(),context.grants.end(),[&](const capability_grant& grant){
   return grant.permission==required && (!grant.resource || *grant.resource==resource);
 });
}
// NOTE: has_grant only checks locally supplied declarations. Never treat it
// as actual authorization without validating the trusted caller/principal,
// expiry, target, and provenance server-side.
} // namespace engineering
} // namespace gut
