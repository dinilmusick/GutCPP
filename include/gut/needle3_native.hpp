#pragma once
// Opt-in Needle 3 inference via the real needle-rs 0.3.1 C ABI.
// Runtime pin: geekgineer/needle-rs@4de50494fd60f417b24c37e4d972f95d128f8a0f.
// Do not include this header unless the caller links libneedle_c.
#include "gut/json.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

extern "C" {
struct NeedleV3Handle;
NeedleV3Handle* needle_v3_load(const char*);
void needle_v3_free(NeedleV3Handle*);
std::size_t needle_v3_num_layers(NeedleV3Handle*);
char* needle_v3_run_json(NeedleV3Handle*,const char*,const char*);
char* needle_v3_generate(NeedleV3Handle*,const char*,const char*,
                        std::size_t,float,std::uint64_t,bool,bool);
void needle_free_str(char*);
const char* needle_last_error(void);
}

namespace gut {
namespace needle3_native_detail {
inline std::string c_safe(std::string_view text,const char* kind,bool allow_empty=false){
  if((!allow_empty&&text.empty())||text.find('\0')!=std::string_view::npos)
    throw std::invalid_argument(std::string(kind)+" must be a nonempty NUL-free UTF-8 string");
  (void)utf8_scalars(text);
  return std::string(text);
}
inline std::runtime_error native_error(const char* action){
  const char* error=needle_last_error();
  return std::runtime_error(std::string("Needle 3 ")+action+" failed: "+
                            (error&&*error?error:"native runtime returned null"));
}
struct handle_deleter { void operator()(NeedleV3Handle* p)const noexcept {
  if(p)needle_v3_free(p);
}};
struct string_deleter { void operator()(char* p)const noexcept {
  if(p)needle_free_str(p);
}};
using native_string=std::unique_ptr<char,string_deleter>;
}

// Needle3Session is a real, stateful native inference session, not a callback
// wrapper. It is movable, not copyable, and must not be used concurrently.
class Needle3Session {
 public:
  explicit Needle3Session(std::string_view model_file) {
    const auto path=needle3_native_detail::c_safe(model_file,"model file");
    model_.reset(needle_v3_load(path.c_str()));
    if(!model_)throw needle3_native_detail::native_error("load");
    if(!needle_v3_num_layers(model_.get()))throw std::runtime_error("Needle 3 model has no layers");
  }
  Needle3Session(const Needle3Session&)=delete;
  Needle3Session& operator=(const Needle3Session&)=delete;
  Needle3Session(Needle3Session&&)=default;
  Needle3Session& operator=(Needle3Session&&)=default;
  std::size_t layers()const noexcept{return model_?needle_v3_num_layers(model_.get()):0;}

  // Result is a JSON array of genuine model tool calls (or [] for abstention).
  // nullopt means the native completion did not produce tool_call markers.
  std::optional<json_value> tool_calls(std::string_view prompt,
                                        const json_value::array& tools) {
    auto p=needle3_native_detail::c_safe(prompt,"prompt");
    auto definitions=stringify_json(json_value(tools));
    if(!model_)throw std::logic_error("Needle 3 session was moved or closed");
    needle3_native_detail::native_string raw(
        needle_v3_run_json(model_.get(),p.c_str(),definitions.c_str()));
    if(!raw)throw needle3_native_detail::native_error("tool inference");
    if(!*raw)return std::nullopt;
    auto parsed=parse_json(raw.get());
    if(!std::holds_alternative<json_value::array>(parsed.data))
      throw std::runtime_error("Needle 3 returned non-array tool call JSON");
    return parsed;
  }

  // Full generation, including any <think> or <tool_call> blocks.
  std::string generate(std::string_view prompt,const json_value::array& tools,
                       std::size_t max_new_tokens=256,bool constrain=true,bool kv_int8=false) {
    auto p=needle3_native_detail::c_safe(prompt,"prompt");
    auto definitions=stringify_json(json_value(tools));
    if(!model_)throw std::logic_error("Needle 3 session was moved or closed");
    if(max_new_tokens>4096)throw std::invalid_argument("token limit exceeds bounded native API");
    needle3_native_detail::native_string raw(needle_v3_generate(
        model_.get(),p.c_str(),definitions.c_str(),max_new_tokens,0.0f,0,
        constrain,kv_int8));
    if(!raw)throw needle3_native_detail::native_error("generation");
    return std::string(raw.get());
  }

 private:
  std::unique_ptr<NeedleV3Handle,needle3_native_detail::handle_deleter> model_;
};
} // namespace gut
