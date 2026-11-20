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


## Verified GutTS differential check

`scripts/verify-upstream-differential.mjs` executes 95 deterministic cases across all 33 portable GutTS API IDs using the exact pinned GutTS build and the strict native JSON-lines runner; `.github/workflows/golden.yml` runs this integration. A discovered `compact` mismatch is corrected in C++17 and covered by typed and JSON tests. Passing this finite corpus does **not** imply full model/browser/Unicode parity.


## Registry and JSON identity differential verification

Pinned GutTS CI compares all 38 public API definitions (33 executable utility IDs plus five non-executable model API metadata descriptors), then executes 100+ utility fixtures against both C++ and the exact TypeScript runtime. Native JSON `unique` preserves independently parsed object and array identities. Model operations fail explicitly without a native provider; the library makes no claim of machine-learning inference parity.


## Nested JSON properties

`gut::json_has_path`, `json_pick_paths` and `json_omit_paths` provide bounded, value-owned nested property operations for the GutTS `hasOwn`/`pick`/`omit` APIs; dotted paths and bracket array indices are supported, and the pinned TypeScript differential workflow checks them. This does not simulate JavaScript prototypes or object reference sharing.


## Native semantic-input orchestration (C++17)

`gut/semantic_workflow.hpp` implements real deterministic resolution, ranking orchestration, structured-output unwrapping, candidate confidence/margin checks, ambiguity and clarification, cancellation, events, confirmation flags and bounded thread-safe cache. It requires explicit caller-provided MiniLM/Ettin/Needle model functions; missing backends fail closed. It does **not** fabricate inference, embeddings, relevance scores or download model artifacts. Full native inference and browser-specific behavior remain separate parity gaps. The `semantic_workflow_test` validates the orchestration behavior with injected deterministic callbacks, not live inference.


## Decimal rounding parity

`gut::round_to` now performs C++17 locale-independent decimal-exponent shifting to reproduce Lodash round on the verified finite numeric corpus, including 1.005, 2.675, negative half-ties and signed negative zero. Both native unit tests and the executed pinned GutTS differential workflow guard it. Extremely large precision and JavaScript Number-to-string corner cases are documented deviations.


## Ettin neural scoring head

Header `gut/ettin_head.hpp` implements the real float32 dense/GELU/LayerNorm/logistic Ettin scoring head, with local Safetensors F32 parsing and filesystem loader. Native CTest tests real file loading, malformed input and tensor guards. CI executes a 15-fixture differential against the compiled pinned GutTS head function. A real transformer model must supply hidden states—the header does not generate embeddings, tokenize input, or claim model inference completeness.


## Native Google Drive v3 REST adapter

`include/gut/google_drive.hpp` provides a real C++17 Google Drive HTTP implementation (not a placeholder). It supports list/get metadata, file media download/export, metadata create/rename, simple binary media create/update, copy and delete. Authentication requires a caller-supplied **OAuth bearer token provider**. Production requests are pinned to `https://www.googleapis.com`; a verified loopback-only origin is opt-in for tests. This header reuses the injected native HTTP transport without Node.js or browser dependencies. With `gut::curl_http_adapter`, pass the adapter via a lambda. The curl transport now supports POST, GET, PUT, PATCH, DELETE and HEAD; bearer-only requests use a **separate cookie-free CURL handle**, preventing cross-client BFF session leakage. Strict local unit, AddressSanitizer/UndefinedBehaviorSanitizer and real loopback HTTP integration tests are included. The tests do not claim authenticated access to a live Google account, full Google SDK compatibility, large-file resumable uploads or cross-cloud synchronization.


## Native AWS S3 / S3-compatible object storage (opt-in)

`include/gut/s3.hpp` implements actual native C++17 S3 requests with **AWS SigV4 HMAC-SHA256** using OpenSSL Crypto and an injected HTTP transport. It covers all seven distinct operations re-exported by GutTS's `storage/s3` entrypoint: `GetObject`, `HeadObject`, `ListObjectsV2`, `PutObject`, `CopyObject`, `DeleteObject`, and `DeleteObjects` (the latter includes the required Content-MD5). Optional custom HTTPS endpoints support S3-compatible services such as Backblaze B2; plaintext HTTP requires explicit exact-loopback test authorization. Access/secret/session credentials are caller-supplied and redacted from errors. Unit tests check hashing/signature building, malformed inputs and boundary behavior; a real libcurl HTTP integration suite uses an independently implemented Python HMAC verifier over a loopback server, including cookie isolation, binary payloads and copy/multi-delete semantics. This is not the AWS SDK's entire credential provider chain, multipart streaming or live AWS-account certification. Enable `-DGUTCPP_TEST_S3=ON` with libcurl and OpenSSL development libraries; the portable core remains header-only without mandatory runtime libraries.


## Actual MiniLM post-embedding numerical parity

`gut::minilm_cosine` and `gut::minilm_similarity_from_pooled_embeddings` accept real native float32 pooled feature-extractor tensors and perform the arithmetic from pinned `GutTS/src/models/runtimes/transformers.ts`. The exact compiled pinned GutTS *private* cosine function is exposed in a CI-only untracked sibling module and executed on the same 32+ deterministic float32 vector pairs as the strict C++17 JSON-Lines runner; the workflow checks numerical results and special NaN classifications. CTest also checks invalid tensor sizes, zeros, negatives and finite precision, with installed-header consumer coverage. No embeddings are invented and no transformer model is loaded: actual MiniLM tokenizer, embedding extraction, model weights and inference remain external dependencies that must be implemented/verified separately before claiming model parity.
