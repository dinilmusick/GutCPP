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

## Native application authentication (BFF)

Include `gut/application_auth_client.hpp` for a fail-closed native application-authentication client. It validates local navigation/return paths, constructs same-origin cookie-aware session and logout requests, rejects malformed session JSON, verifies logout, and never forwards bearer credentials. Supply a real same-origin HTTP transport and a navigation callback from the host. The standalone C++17 regression includes successful and negative request paths; remote Luth authentication is **not** implied.

## Direct native Luth protocol and optional libcurl HTTP

`#include <gut/luth_client.hpp>` implements synchronous C++17 Luth registration, session/refresh, sign-in/sign-up, Google flow initiation, logout verification, retry and endpoint failover. Supply a secure token store and HTTP transport. Optional `#include <gut/curl_http.hpp>` supplies a real libcurl transport with verified TLS, redirect prevention, memory-only cookies and response size limits. The portable `gutcpp::gutcpp` target does **not** require libcurl.

```sh
cmake -S . -B build-http -DCMAKE_BUILD_TYPE=Release -DGUTCPP_TEST_CURL=ON
cmake --build build-http --parallel
ctest --test-dir build-http --output-on-failure
```

Actual loopback HTTP tests cover cookie-based BFF sessions and direct Luth requests, not a real production OAuth account or ML inference. The pinned GutTS parity map lists outstanding semantic differences.

## Model resource lifecycle and server adapter

`gut/model_library.hpp` supplies a native C++17 `ModelLibrary` wrapping a user-supplied `model_runtime<T>` with real load/unload and thread-safe residency. `model_library_test` loads real file content from disk and checks deterministic cleanup, but **does not** stand in for executing a machine-learning model. `gut/server_auth.hpp` isolates the native `APP_URL` environment lookup. Plain HTTP in the libcurl adapter is limited to exact loopback hostnames and valid TCP ports; prefixes such as `localhost.attacker.example` are rejected.

## Executable upstream-vs-native differential checks

The `golden.yml` CI job now builds `gutcpp_differential_runner`, compiles immutable GutTS from its pinned SHA and executes the two runtimes on identical JSON-Lines test cases. It asserts every one of the 33 non-model registry IDs appears in the fixture corpus, compares real values and exception outcomes, and fails on divergence. Reproduce after checking out and compiling the pinned GutTS reference:

```sh
cmake -S . -B build-differential -DBUILD_TESTING=OFF -DGUTCPP_DIFFERENTIAL_RUNNER=ON
cmake --build build-differential --target gutcpp_differential_runner
node scripts/verify-upstream-differential.mjs .upstream ./build-differential/gutcpp_differential_runner
```

This regression corpus does not assert that untested source contracts, machine-learning inference, or cloud integrations have reached full parity.
