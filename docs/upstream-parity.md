# GutCPP parity and portability matrix

Source pinned **verbatim** to `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c` (GutTS 2.2.3); native baseline `dinilmusick/GutCPP@7fc8ee9e9763ba4bcf1c32ee56621de3be886616`.

This document intentionally distinguishes **implemented** native contracts from *verified differential equivalence*. Implementation and passing unit tests do not prove 100% upstream parity. All portable package entry points from pinned `package.json` and root `src/index.ts` have been categorized.

| GutTS public export / symbol | C++17 API | Implementation | Executable test | Status |
|---|---|---|---|---|
| arrays: `compact`, `unique`, `chunk`, `flatten`, `last` | `gut::compact`, `unique`, `chunk`, `flatten`, `last` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Native implemented; differential pending |
| objects: `hasOwn`, `pick`, `omit` | `gut::has_own`, `pick`, `omit` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Native map implemented; JS nested-path semantics differ |
| objects: `isPlainObject` | C++ typed `std::map` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Not generally portable; JS object prototype distinctions absent |
| numbers: `clamp`, `inRange`, `roundTo`, `isFiniteNumber`, `sum` | `gut::clamp`, `in_range`, `round_to`, `is_finite_number`, `sum` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Native implemented; decimal rounding differential pending |
| strings: `isBlank`, `isNonBlank`, `startsWithAny`, `endsWithAny`, `truncateWords` | `gut::is_blank`, `is_non_blank`, `starts_with_any`, `ends_with_any`, `truncate_words` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Implemented; UTF-8 scalar and UTF-16 differences noted |
| strings: `isEmail`, `capitalize`, `camelCase`, `kebabCase`, `snakeCase`, `truncate` | `gut::is_email`, `capitalize`, `camel_case`, `kebab_case`, `snake_case`, `truncate` | `include/gut/strings.hpp` | `tests/strings_test.cpp` | **Implemented subset**: strict UTF-8, ASCII email and case behavior; validator.js/Lodash full Unicode and email equivalence not yet certified |
| json: `JsonValue`, `isJsonValue`, `parseJson`, `tryParseJson`, `stringifyJson` | `gut::json_value`, `is_json_value`, `parse_json`, `try_parse_json`, `stringify_json` | `include/gut/json.hpp` | `tests/json_test.cpp` | Native implemented; deliberate strict UTF-8/depth rules and lexical differences |
| urls: `joinUrl` | `ultrst::gut::join_url` | `include/gutcpp/url.hpp` (existing PR #2) | `tests/url_test.cpp` | Existing PR preserved, integration not independent parity proof |
| urls: `isUrl`, `getQueryParam`, `setQueryParam` | `gut::is_url`, `get_query_param`, `set_query_param` | `include/gut/net.hpp` | `tests/net_test.cpp` | Native implemented for absolute HTTP(S), not complete WHATWG URL parser |
| urls/safety: `normalizeStoragePath`, `encodeSafePathSegments` | `gut::normalize_storage_path`, `encode_safe_path_segments` | `include/gut/storage_path.hpp` (existing PR #4) | `tests/storage_path_test.cpp` | Existing PR preserved; UTF-8 edge cases checked |
| urls/safety: `resolveTrustedApiUrl` | `gut::resolve_trusted_api_url` | `include/gut/net.hpp` | `tests/net_test.cpp` | Native HTTPS/relative path guard implemented; WHATWG parser differences possible |
| RFC3986 component encoding | `gut::percent_encode_component` | `include/gut/percent_encoding.hpp` (existing PR #3) | `tests/percent_encoding_test.cpp` | Existing PR preserved; this is not JS `encodeURIComponent` (`!'()*` differ) |
| auth: `isIdentityKind`, `parseIdentityId`, `identityRef`, `parseIdentityRef`, `parseLegacyApplicationId` | `gut::is_identity_kind`, `valid_identity_id`, `parse_identity_ref`, `parse_legacy_application_id` | `include/gut/portable.hpp` | `tests/portable_test.cpp` | Native ID and wire reference validation implemented |
| auth: OAuth client registration, redirect URI validators | `gut::validate_oauth_client_registration`, `validate_oauth_redirect` | `include/gut/auth_contracts.hpp` | `tests/auth_contracts_test.cpp` | Native shape validator implemented; WHATWG URL parsing limits apply |
| auth: capability publication and invocation budget validators | `gut::validate_capability_descriptor`, `validate_scoped_invocation`, `check_invocation_budget`, `advance_invocation_budget`, `consume_invocation_retry`, `validate_invocation_against_descriptor` | `include/gut/auth_contracts.hpp` | `tests/auth_contracts_test.cpp` | Native metadata validation implemented; NEVER authentication or consent authority |
| auth: `ApplicationAuthClient` (backend-for-frontend) | `gut::application_auth_client` | `include/gut/application_auth_client.hpp` | `tests/application_auth_client_test.cpp` | **Native request/response logic implemented**: login navigation, local-return-path validation, session schema verification, cookie-only transport contract and logout recheck. Host-provided real HTTP transport required; asynchronous JavaScript fetch semantics not copied. |
| auth: `LuthAuthClient` (direct Luth protocol) | `gut::luth_client` + optional `gut::curl_http_adapter` | `include/gut/luth_client.hpp`, `include/gut/curl_http.hpp` | `tests/luth_client_test.cpp`, `tests/luth_curl_integration_test.cpp` | **Native protocol implemented**: registration, cached registration, email sign-in/up, Google sign-in initiation, session refresh, token store, failover/retries, logout revocation. Optional real libcurl loopback integration. JavaScript async/browser state and production Luth deployment are not certified. |
| auth: published Luth type declarations (`LuthUser`, `LuthSession`, configuration, errors and token store) | `gut::luth_*`, `luth_auth_error`, `luth_auth_token_store` | `include/gut/luth_types.hpp` | `tests/luth_types_test.cpp` | **Portable typed records implemented**; Luth networking / server-verified sessions still require a real HTTP adapter |
| `engineering` pure data contracts | `gut::engineering::*` typed records/enums, validation | `include/gut/engineering.hpp` | `tests/engineering_test.cpp` | **Typed portable declarations implemented**; no native execution adapter or authoritative grants |
| `api` registry/definitions/execution, FOD graph | `gut::portable_api_definitions`, `execute_gutts_api`, `gutts_api_to_fod_node` | `include/gut/api.hpp` | `tests/api_test.cpp` | **33 utility IDs implemented** with real execution and per-ID fixtures. Metadata schemas/descriptions and object identity, URL and Unicode semantics still differ; 5 model APIs absent |
| `models` lifecycle: `ModelResidency.run`, `isLoaded`, `activeUsers` | `gut::model_residency<T>` | `include/gut/model_residency.hpp` | `tests/model_residency_test.cpp` | **Implemented synchronous C++17 analogue** with mutex/condition variable, at-most-once load/unload per overlapping usage, and propagated failure; TS async promise semantics differ |
| `models`: `ModelLibrary.withModel`, `ModelRuntime.load/unload` | `gut::model_library<T>`, `gut::model_runtime<T>` | `include/gut/model_library.hpp` | `tests/model_library_test.cpp` | **Native synchronous resource lifecycle implemented**, caller-owned runtime performs real loading/unloading; tested with real filesystem-backed load/unload; model inference not fabricated |
| `models` metadata registry `guttsModelDefinitions`, `guttsModelById` | `gut::gutts_model_definitions`, `gutts_model_by_id` | `include/gut/model_registry.hpp` | `tests/model_registry_test.cpp` | **All eight metadata records implemented** from pinned source; no inference claimed |
| `semantic-input`: `resolveDeterministicSemanticInput`, `SemanticInputCache` | `gut::resolve_deterministic_semantic_input`, `semantic_input_cache` | `include/gut/semantic_input.hpp` | `tests/semantic_input_test.cpp` | **Deterministic subset implemented**; decimal/regex and Unicode edge semantics differ from JS |
| `models` inference classes, `semantic-input` ML-assisted resolution | — | — | — | **Missing**: real native ML runtime and model artifacts required; no mocked model inference or semantic-ranking parity claimed |
| `server/auth.resolveServerRuntimeAppUrl` | `gut::resolve_server_runtime_app_url` | `include/gut/server_auth.hpp` | `tests/server_auth_test.cpp` | **Native environment adapter implemented**: trims APP_URL; isolated from portable core. |
| `browser`, `worker`, `server`, `storage`, `storage/s3`, `storage/google-drive`, `server/models`, `server/storage` | — | — | — | Platform-specific; portable data contracts may be ported, actual network/runtime adapters need separate native implementation |
| error/result contract | `gut::Result<T>`, `gut::Error` | `include/gut/result.hpp` (existing PR #1) | `tests/result_test.cpp` | Native value/error contract; upstream TypeScript exceptions differ |

## Known semantic differences and safety limits

- `std::vector<T>` preserves copy/move/lifetime guarantees but is not JavaScript array: `unique` uses C++ equality rather than SameValueZero; `flatten` consumes a uniform vector of vectors; JS `undefined` maps to `std::optional`.
- `std::map` is sorted. JavaScript object insertion order and lodash nested property path handling are **not** reproduced. `isPlainObject` has no meaningful equivalent for statically typed `std::map`.
- JSON parser rejects non-scalar UTF-8 and malformed escapes, enforces a 128-nesting cap and rejects overflowing finite double values. JS `JSON.parse` and `JSON.stringify` have different escape ordering, number rendering and UTF-16 surrogate behavior.
- Native URL parser intentionally limits absolute URLs to HTTP(S), is not the WHATWG URL implementation, and must not be used for arbitrary OAuth redirect verification without additional differential tests.
- C++ `round_to` is floating multiplication/floor based rather than lodash's exponent string shifting; extreme precision cases may disagree.
- OAuth/capability/invocation shape validators are never substitutes for verifying signed identities, workspace membership, grants, live UI consent, or transport authorization.
- `is_email` conservatively handles unquoted ASCII mailboxes and DNS labels only. Lodash tokenization, diacritics, contractions, grapheme clusters and Unicode casefolding are not all replicated by ASCII-aware `capitalize`, `camel_case`, `kebab_case`, `snake_case` and code-point-based `truncate`. Invalid UTF-8 is rejected. Never claim complete validator.js/Lodash semantics.
- `execute_gutts_api` has real execution for all 33 non-model registry IDs with `std::optional<json_value>` to distinguish JavaScript `undefined` from JSON null. Object identity equality, property order, nested-path lookup, WHATWG URL behavior and numeric rounding differ at documented boundaries; this is an executable native API bridge, not full JS identity/runtime emulation. The FOD adapter creates typed port definitions, not an agent or graph scheduler.
- The model registry contains the eight exact pinned model identities, versions, approximate sizes, capabilities, and URIs. It is metadata only and cannot produce text, embeddings, audio transcripts, or rerank scores. The native deterministic semantic resolver handles typed literal, enum, basic format, and regex inputs and a synchronized bounded LRU cache; it does NOT run ML models or rank candidates. Native decimal/regex semantics and cancellation differ from JavaScript.
- Luth auth type declarations and token-store interface are data contracts; no HTTP requests, cookies, registration, authentication or authoritative authorization occurs in this header.
- `model_residency<T>` serializes loading/unloading, shares a model across concurrent `run` calls, unloads after final user, rejects reference return types and preserves the primary operation exception while retaining a secondary cleanup exception. The host must keep the residency object alive during operations. Unlike GutTS, loading/operation/unloading are synchronous; a real model runtime must be supplied by the caller.
- `gut::engineering` carries typed workspace/repository/execution/verification/grant/provenance structures; `has_grant` only matches declared data and is not a security authorization.
- No runtime dependency on Node.js, React or browser APIs. `gutcpp::gutcpp` requires C++17, uses header-only values and stack/resource-automatic ownership; `std::bad_variant_access` and `std::invalid_argument` are documented exceptions. No-exception builds are not supported by these parsing APIs.

## Golden fixture provenance and differential verification

- `scripts/generate-upstream-golden.mjs` runs actual GutTS functions from a compiled checkout at the exact pinned SHA, **not a hand-copied TypeScript approximation**.
- `.github/workflows/golden.yml` independently checks out GitTS at `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`, installs frozen dependencies, compiles it, runs the fixture generator, and byte-compares its deterministic output to the checked-in snapshot in `testdata/upstream-golden/gutts-3e2cbc0.json`.
- The snapshot contains **28 actual GutTS execution cases** (23 normal and 5 negative), covering UTF-8 whitespace, URI queries/path safety, JSON, arrays, numeric values, and tagged identities. Its source is the immutable GitHub Actions job log, not a locally invented golden file.
- `tests/upstream_golden_test.cpp` loads that snapshot and checks the equivalent C++ operations. The negative cases verify rejection, but JavaScript and C++ exception *types* differ, so error type equality is not claimed.
- These 28 examples are **not exhaustive**; remaining symbols and adversarial cases require additional fixtures.

## Build and install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix ./installed
cmake -S consumer -B consumer-build -DCMAKE_PREFIX_PATH="$PWD/installed"
cmake --build consumer-build && ctest --test-dir consumer-build --output-on-failure
```

The second native pass adds string operations, 33 executable GutTS utility API IDs/FOD ports, thread-safe model residency, and typed engineering records. The third pass adds **8 pinned model metadata records**, deterministic semantic-input resolution and its synchronized LRU cache, and Luth authentication data structures with explicit token-store interface. Exact-head cross-platform integration remains the release gate; **total upstream runtime parity is still incomplete** pending real ML and platform transport adapters and documented semantic differences.

## Native application-authentication host boundary

`gut::application_auth_client` implements the application-facing `/api/auth/oidc` protocol with a caller-injected same-origin, cookie-aware HTTP transport and navigation callback. The application must honor `same_origin_credentials` and `no_store`, retain all OAuth tokens/state/verifiers in its own server or HttpOnly cookie store, and never treat a supplied request as proof of authorization. `get_session` requires an authenticated user/session JSON envelope; HTTP 401 returns unauthenticated. `sign_out` POSTs an empty object and checks session revocation. A host transport is necessary for live HTTP; BFF protocol tests validate dispatch semantics, not live server integration. Native Luth protocol and optional real HTTP are implemented in the adjacent section; **deployed** Luth identity and actual model inference remain unverified.

## Actual native HTTP transport boundary

`include/gut/curl_http.hpp` is **opt-in** and uses libcurl with TLS verification, redirect prevention, in-memory cookie storage and bounded responses; plaintext HTTP is confined to loopback for development. The portable core remains free of libcurl and Node.js dependencies. `tests/curl_loopback_server.py` drives two real loopback HTTP tests for cookie-based BFF session/logout and direct Luth registration/email sign-in/session/logout. These demonstrate native HTTP protocol and cookie handling, **not** OAuth provider authentication, remote workspace grants, production deployed Luth credentials, or ML inference parity. Native `gut::luth_client` uses caller-owned token store, explicit native transport, and synchronous APIs; JavaScript fetch, promise, sessionStorage and abort semantics remain differences.

## Exact loopback-host hardening (2026-11-20)

The optional native HTTP adapter and Luth origin validator require exact `localhost`, `127.0.0.1` or `[::1]`, optionally followed by a decimal TCP port in 1..65535. A hostname-prefix such as `localhost.attacker.example` or `127.0.0.1.evil.example` **must never** qualify for plaintext HTTP. The adapter rejects these values before DNS resolution. `tests/luth_client_test.cpp` and the real `tests/curl_loopback_test.cpp` exercise positive and negative authority cases. TLS verification and redirect prevention remain mandatory outside exact loopback.

## Runtime-to-runtime API differential harness

`tests/api_differential_runner.cpp` exposes a strict JSON-Lines C++17 executable that invokes `gut::execute_gutts_api` on real input records. `scripts/verify-upstream-differential.mjs` imports and **executes** `executeGuttsApi` from the immutable compiled `GutTS@3e2cbc0` checkout in GitHub Actions, compares parsed JSON results, distinguishes JSON null from JavaScript `undefined`, and classifies actual exception outcomes. The harness automatically requires coverage for all 33 non-model published utility API IDs and checks positive, negative and boundary cases. `golden.yml` fails on any mismatch; it does not use fake expected model outputs. This is in addition to the 28 pinned execution vectors already committed under `testdata/upstream-golden`.

The harness intentionally compares structural JSON data rather than object-reference identity or raw formatting. It does not prove unbounded semantic equivalence; untested Unicode, URL, native model, and platform operations remain separate requirements.


## 2026-11-20 differential remediation

The initial exact-pinned GutTS execution corpus returned 94/95 matching JSON-line results across all 33 non-model utility IDs. The discrepancy was `gutts.arrays.compact`: the actual pinned TypeScript implementation delegates to Lodash `compact` (which removes all falsy values), not merely nullish values as its descriptive API metadata implies. Both native `execute_gutts_api` and the typed `compact<T>` now discard false, zero, empty strings, and null, while retaining truthy nonempty scalars and empty object/array containers. Tests exercise both paths, and the GitHub golden workflow reruns the entire pinned corpus. Model inference, broad Unicode and WHATWG URL semantics are still separate, explicitly unverified work.


## 38 pinned public API definitions and JavaScript identity (2026-11-20)

`gut::gutts_api_definitions()` now represents the full pinned 38-entry API registry, including five **metadata-only** model APIs. `portable_api_definitions()` remains the 33 runnable non-model operations. Attempted model dispatch fails explicitly with the same “metadata but no embedded runtime adapter” contract as pinned GutTS, not fake inference. The golden CI now compares names, IDs, required parameters, schema types, output types and deterministic/purity flags across all 38 TypeScript and C++ descriptors, alongside 100+ executed utility fixtures.

`gutts.arrays.unique` now uses JavaScript SameValueZero for JSON scalars while retaining distinct array/object values even if structurally identical: JSON parsing cannot encode JavaScript shared-reference identity, so separate object instances must not be deduplicated on value. The typed generic `unique<T>` intentionally continues native value equality, with explicit C++ value semantics. Pinned model inference still requires a real native runtime; declaring metadata does not confer executable inference parity.


## Lodash nested path handling (2026-11-20)

`include/gut/object_paths.hpp` implements native JSON path traversal, selection and removal for `hasOwn`, `pick`, and `omit`, matching the pinned Lodash-style dotted keys and bracket-indexed arrays for ordinary JSON. A literal root property name takes precedence over its parsed dotted form. Arrays are value-owned: dropping an indexed element leaves JSON null at that position, analogous to a JavaScript array hole serialized to JSON. Path depth and index sizes are bounded to prevent allocation exhaustion, and prototype-like keys are rejected for mutations. This is an explicit native JSON subset; JavaScript prototype chains, exotic objects, inherited properties and shared mutable references are **not** implemented. The exact GutTS-vs-C++ differential suite now exercises nested paths, bracket indexing and literal-key precedence alongside the existing 38/38 registry check.


## C++17 semantic input orchestration (2026-11-20)

The `semantic_workflow.hpp` entrypoint `gut::resolve_semantic_workflow` implements the non-inference semantic-input control flow: deterministic schema detection, cache lookup/eviction and per-model-revision invalidation, bounded candidate ranking and relevance margin admission, output-envelope unwrapping, typed resolution states, ambiguity/clarification, cancellation, event/progress hooks and explicit failure propagation. Backends are **injected** in `semantic_workflow_models`; this is not native Needle/MiniLM/Ettin implementation, and callback-based orchestration unit tests are not counted as inference parity. Synchronous C++17 semantics, a reduced prompt/context schema, absent JS event-loop/AbortSignal/Promise behavior and no model artifacts are explicit differences. Native ML inference remains outstanding.


## Numeric rounding parity (2026-11-20)

`gut::round_to` now implements Lodash-style decimal exponent shifting for finite values (instead of multiplying by powers of ten), preserving JavaScript `Math.round` half-ties toward positive infinity and signed negative zero. Positive and negative decimal-tie cases are checked against the exact pinned TypeScript runtime in `scripts/verify-upstream-differential.mjs`. Extreme precision above 308 and JavaScript Number string-format edge cases remain explicitly outside full equivalence. C++17 locale-independent floating `charconv` is required.


## Native Ettin scoring head (2026-11-20)

`gut::compute_ettin_head_scores` implements the **actual** pinned GutTS Ettin post-transformer scoring head (dense projection, tanh-GELU, layer normalization, final dense/logistic), using float32 intermediate values. `parse_safetensors_f32` and `load_ettin_head_weights` parse/read real little-endian IEEE-754 Safetensors head files with header, offset, dtype and shape bounds. This is usable with real transformer hidden states; no simulated inference or fake embeddings are generated. A 15-case deterministic cross-language runner executes the compiled *real* `GutTS/src/models/runtimes/ettin-head.ts` function at the immutable source commit and compares numeric outputs within 2e-6, while native CTest validates malformed Safetensors and filesystem loading. **Still missing:** native transformer backbone inference/tokenizer and model artifact lifecycle; head parity must not be represented as full Ettin model parity.


## Google Drive v3 native storage adapter (2026-11-20)

GutTS `src/storage/google-drive.ts` uses an optional Node-only `googleapis` adapter, returning an opaque official Drive v3 client. `GutCPP` provides `gut::google_drive_client` in `include/gut/google_drive.hpp`, with real native transport-backed Drive REST operations: `list_files`, `get_metadata`, `download_media`, `export_file`, `create_metadata`, `upload_media`, `update_media`, `rename_file`, `copy_file`, and `delete_file`. Executable tests: `tests/google_drive_test.cpp` (wire contract and negative cases) and opt-in `tests/google_drive_curl_test.cpp` + `tests/google_drive_loopback_server.py` (real libcurl HTTP and binary body transfer). CMake installed consumer includes the new public header. OAuth bearer retrieval remains a caller responsibility; production origin is restricted to `https://www.googleapis.com`. Cookie-free bearer transport is enforced separately from reusable same-origin session cookies. These tests prove correct actual HTTP dispatch against a loopback server, not live Google OAuth, official SDK breadth, large-file resumable upload, or exact cross-language parity with the `googleapis` implementation. Full model inference, S3 SDK, and Unicode/WHATWG gaps remain outstanding.


## Opt-in native S3 object-storage adapter (2026-11-20)

Pinned `GutTS/src/storage/index.ts` and `storage/s3.ts` re-export `S3Client` and the `GetObject`, `HeadObject`, `ListObjectsV2`, `PutObject`, `CopyObject`, `DeleteObject`, and `DeleteObjects` commands from the AWS SDK. `include/gut/s3.hpp` now implements all **seven wire operations** as native C++17 methods backed by real HTTP and OpenSSL SigV4: `s3_client::{get_object,head_object,list_objects_v2,put_object,copy_object,delete_object,delete_objects}`. The credentials provider is injected and never persisted or logged. The canonical request includes signed host/date/sha256/security-token headers and SHA-256 payload hash, with HMAC chaining through date, region, service and `aws4_request`; DeleteObjects adds correctly encoded XML and Content-MD5. Test vectors and negative cases are in `tests/s3_sigv4_test.cpp`; `tests/s3_curl_test.cpp` and `tests/s3_loopback_server.py` execute actual signed requests and independently verify each HMAC over localhost. `native-http.yml` runs them with optional OpenSSL/libcurl, compiler warnings as errors, an independently compiled installed-header consumer and Linux AddressSanitizer/UndefinedBehaviorSanitizer. This does **not** claim full AWS SDK API surface, managed credentials, multipart streaming, cross-region endpoint discovery, XML response abstraction, live AWS permissions or production cloud interoperability. It advances native implementation parity without fabricating a model or service.


## MiniLM cosine after real embeddings (2026-11-20)

Native C++17 header `include/gut/minilm_math.hpp` implements exact ordered double-precision `dot`, `leftNorm`, `rightNorm` accumulation, zero-filling absent right components, and `dot / sqrt(leftNorm*rightNorm)` from the pinned private `cosine` in GutTS `models/runtimes/transformers.ts`. It also splits paired float32 embedding tensors into two equal halves with identical validation. Tests: `tests/minilm_math_test.cpp` with strict CTest, and `scripts/verify-minilm-cosine.mjs` executing real compiled pinned TypeScript mathematics against `tests/minilm_math_runner.cpp` for 32+ numeric vector cases. **Not full MiniLM model parity**: generating true pooled embeddings with the actual transformer tokenizer/weights, lifecycle and CPU inference remains unimplemented.


## Optional WHATWG native URL parity via Ada (2026-11-20)

The pinned `src/urls/url-operations.ts` delegates `isUrl`, `getQueryParam`, and `setQueryParam` to JavaScript `URL` and `URLSearchParams`, whose semantics extend beyond HTTP(S). `include/gut/ada_url.hpp` implements `gut::is_url_whatwg`, `get_query_param_whatwg` and `set_query_param_whatwg` using the real Ada WHATWG URL parser at fixed source commit `812b97865c000325ff6b1cac48e89530a919fa7b`. The opt-in dependency is isolated from portable core builds. `tests/ada_url_test.cpp` tests native cases; `tests/ada_url_runner.cpp` and `scripts/verify-ada-url.mjs` compare actual compiled pinned GutTS source outputs for dozens of URL/query cases. Independent Linux/macOS/Windows C++17 jobs test Ada integration. Use only after exact-head certification; this does not replace stricter protocol/origin guards for authentication or storage and does not address native ML inference.
