# Native SmolLM2-135M-Instruct real text generation (opt-in C++17)

The pinned GutTS model family includes SmolLM2, but metadata/type declarations are not inference.
This optional implementation uses the **actual** transformer weights through the genuine
[llama.cpp](https://github.com/ggml-org/llama.cpp) in-process native C API;
no subprocess, JavaScript worker, network inference API or hardcoded completion.

## Immutable provenance

- TypeScript reference: `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`
- Native runtime: `ggml-org/llama.cpp@40b7a546defe1f60edd1338c719552ffc58c89a0`
- Real converted model: `bartowski/SmolLM2-135M-Instruct-GGUF`, commit
  `bd44ef957626f890d5d646b1132886bc737bf473`,
  `SmolLM2-135M-Instruct-Q4_K_M.gguf`
- Model SHA-256: `2e8040ceae7815abe0dcb3540b9995eaa1fa0d2ca9e797d0a635ae4433c68c2d`
- Model origin: `HuggingFaceTB/SmolLM2-135M-Instruct` (Apache-2.0);
  this GGUF is quantized by a separate publisher.

## Compile and execute

```sh
cmake -S . -B build-smollm2 -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release \
  -DGUTCPP_ENABLE_SMOLLM2_GGUF=ON -DGUTCPP_SMOLLM2_MODEL_PATH=/models/SmolLM2-135M-Instruct-Q4_K_M.gguf
cmake --build build-smollm2 --target gutcpp_smollm2_native_test --parallel 2
ctest --test-dir build-smollm2 -R smollm2_real_generation --output-on-failure
```

Application CMake consumers link `gutcpp::smollm2_native`, not the
dependency-free `gutcpp::gutcpp` core. Include `gut/smollm2_native.hpp` and
construct `gut::SmolLM2NativeSession(model_path)`. The session loads a
real GGUF model with CPU execution, performs actual tokenization and
autoregressive decoding using greedy sampling, releases model/context/batch/
sampler handles through resource ownership, can be moved (but not copied),
supports `loaded()`, `close()`, and `generate(prompt, max_new_tokens)`.
All inputs and model paths are validated with explicit failure propagation.

## Verification vs. full equivalence

The dedicated GitHub Actions job pins and SHA-256-verifies the original GGUF
bytes, compiles the immutable llama.cpp source and runs genuine inference on
Linux and macOS. The negative tests exercise bad model files, invalid UTF-8,
bounded token limits, closed/moved handles and deterministic repeatability.
A genuine nonempty model completion is required for success.

This adapter uses Q4_K_M rather than the original full-precision model.
Exact logit/byte parity with the pinned GutTS Transformers.js
`SmolLM2.generate` is **not claimed**. The native API is synchronous;
JavaScript promises, browser cancellation and model residency differ.
Absent or invalid model artifacts **fail explicitly**. The normal GutCPP
core build has no dependencies on llama.cpp or GGUF. Successful real model
inference only upgrades the SmolLM2 runtime from unimplemented to
**partially implemented pending full behavioural verification**; it does
not complete the entire Wave 1 or certify Wave 2.
