# GutCPP

Portable, header-only C++17 implementations of applicable `GutTS` data and utility contracts, pinned to upstream `3e2cbc0990ba29830ce91c5d1a1d552675d5334c` for the Wave 1 parity inventory.

## Local build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The project exports installable `gutcpp::gutcpp` CMake target; `consumer/` tests the installed package. See [upstream mapping and explicit missing features](docs/upstream-parity.md). This is an actively developed **partial implementation**, not a claim of full GutTS parity. Upstream JavaScript-compatible URL parsing, Unicode case transformations, runtime transport adapters, and ML services remain unimplemented.

## Native utility/API registry and concurrency

`#include <gut/api.hpp>` exposes 33 executable utility API IDs (`gutts.arrays.*`, `gutts.json.*`, `gutts.numbers.*`, `gutts.objects.*`, `gutts.strings.*`, `gutts.urls.*`). `execute_gutts_api` accepts a JSON object and returns an optional JSON value; an absent optional represents JavaScript `undefined`. `gutts_api_to_fod_node` creates port metadata without starting any agent. `#include <gut/model_residency.hpp>` offers a real, caller-provided, thread-safe synchronous model loader/operation/unloader lifecycle. `#include <gut/engineering.hpp>` provides native typed engineering data contracts.

The second pass deliberately does **not** claim exact Unicode/validator.js/Lodash semantics, WHATWG URL behavior or any ML inference runtime. See `docs/upstream-parity.md` for precise differences.

## Portable model/semantic/auth data contracts

`#include <gut/model_registry.hpp>` returns the eight immutable pinned GutTS model metadata records, **not model execution**. `#include <gut/semantic_input.hpp>` implements deterministic typed semantic input and a synchronized least-recently-used cache; ML-assisted extraction and ranking require real external model runtimes. `#include <gut/luth_types.hpp>` contains Luth user/session/deployment/configuration/error data structures and an application-supplied token store interface. No stubbed login or model inference is exposed.
