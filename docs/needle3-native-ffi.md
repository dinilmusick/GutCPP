# Needle 3 v3 actual native inference (opt-in C++17)

GutCPP's default `gutcpp::gutcpp` is dependency-free; it does **not** silently
claim that the built-in model metadata is executable. For real Needle 3 inference,
link `gutcpp::needle3` through the **optional** C ABI target. This performs
actual local neural-network inference with an explicit `.cact` model file and
**does not** call JavaScript, a remote API, a stand-in callback, or a mocked model.

## Immutable provenance

- Native engine: `geekgineer/needle-rs` at
  `4de50494fd60f417b24c37e4d972f95d128f8a0f` (`needle-c`, MIT).
- Model: `Cactus-Compute/needle3` `needle3.cact` at Hugging Face revision
  `80896a6582ec288abc3f39d0026e931975926c60` (Needle 3 Apache-2.0).
- Pinned GutTS contract: `dinilmusick/GutTS` commit
  `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.
- Runtime ABI: `needle_v3_load`, `needle_v3_run_json`, `needle_v3_generate`,
  `needle_v3_num_layers`, `needle_v3_free`, `needle_free_str`,
  `needle_last_error`. Model handles use `needle_v3_free`; returned strings
  use `needle_free_str`, not C++ `delete` or `std::free`.

```sh
# The commands below are executed on Linux/macOS GitHub Actions against
# pinned native engine and model revisions, not inside the ATAX repository.
git clone https://github.com/geekgineer/needle-rs.git .needle-rs
git -C .needle-rs checkout 4de50494fd60f417b24c37e4d972f95d128f8a0f
cargo build --release --locked --manifest-path .needle-rs/Cargo.toml -p needle-c
hf download Cactus-Compute/needle3 needle3.cact \
  --revision 80896a6582ec288abc3f39d0026e931975926c60 --local-dir .model
cmake -S . -B build-needle3 -DBUILD_TESTING=ON -DGUTCPP_ENABLE_NEEDLE3=ON \
  -DGUTCPP_NEEDLE_C_LIBRARY="$PWD/.needle-rs/target/release/libneedle_c.so" \
  -DGUTCPP_NEEDLE3_MODEL_PATH="$PWD/.model/needle3.cact"
cmake --build build-needle3 --target gutcpp_needle3_native_test
ctest --test-dir build-needle3 -R needle3_real_inference --output-on-failure
```

`#include <gut/needle3_native.hpp>` exposes `gut::Needle3Session(model_path)`.
The class owns the real model handle, cannot be copied, can be moved, and
frees every native output string exactly once. `tool_calls(prompt, tools)`
returns a parsed JSON array; **`[]` is a genuine model abstention** and
`std::nullopt` means the native model produced no tool-call marker. These
must never be conflated. `generate(...)` returns the full native completion,
including reasoning and markers.

The real-model CI loads the downloaded Needle3 weights and checks a live
weather tool call, invalid model paths, embedded NUL input rejection, move
semantics and cleanup, on Linux and macOS. It never counts mocked data as
inference verification. The source-build-only target is not exported to
installed consumers because the external Rust ABI runtime is not bundled.

## Explicit remaining parity gaps

The pinned GutTS implementation also exposes `Needle3.embed` and
`Needle3.complete` with optional JSON-schema extraction; the Needle 3 v3
weights in the pinned Rust C ABI have **no embedding head**, so this wrapper
cannot claim the `embed` operation is implemented. Native Cactus speech
inference for Whistle, full `complete` output contract equivalence, and other
transformer models (SmolLM2, MiniLM, Ettin) remain separately unverified.
This is a concrete native model inference integration, not full GutTS parity.
