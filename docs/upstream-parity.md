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
| auth: `LuthAuthClient`, `ApplicationAuthClient` | — | — | — | **Missing**: real transport, token store, integration |
| auth: published Luth type declarations (`LuthUser`, `LuthSession`, configuration, errors and token store) | `gut::luth_*`, `luth_auth_error`, `luth_auth_token_store` | `include/gut/luth_types.hpp` | `tests/luth_types_test.cpp` | **Portable typed records implemented**; Luth networking / server-verified sessions still require a real HTTP adapter |
| `engineering` pure data contracts | `gut::engineering::*` typed records/enums, validation | `include/gut/engineering.hpp` | `tests/engineering_test.cpp` | **Typed portable declarations implemented**; no native execution adapter or authoritative grants |
| `api` registry/definitions/execution, FOD graph | `gut::portable_api_definitions`, `execute_gutts_api`, `gutts_api_to_fod_node` | `include/gut/api.hpp` | `tests/api_test.cpp` | **33 utility IDs implemented** with real execution and per-ID fixtures. Metadata schemas/descriptions and object identity, URL and Unicode semantics still differ; 5 model APIs absent |
| `models` lifecycle: `ModelResidency.run`, `isLoaded`, `activeUsers` | `gut::model_residency<T>` | `include/gut/model_residency.hpp` | `tests/model_residency_test.cpp` | **Implemented synchronous C++17 analogue** with mutex/condition variable, at-most-once load/unload per overlapping usage, and propagated failure; TS async promise semantics differ |
| `models` metadata registry `guttsModelDefinitions`, `guttsModelById` | `gut::gutts_model_definitions`, `gutts_model_by_id` | `include/gut/model_registry.hpp` | `tests/model_registry_test.cpp` | **All eight metadata records implemented** from pinned source; no inference claimed |
| `semantic-input`: `resolveDeterministicSemanticInput`, `SemanticInputCache` | `gut::resolve_deterministic_semantic_input`, `semantic_input_cache` | `include/gut/semantic_input.hpp` | `tests/semantic_input_test.cpp` | **Deterministic subset implemented**; decimal/regex and Unicode edge semantics differ from JS |
| `models` inference classes, `semantic-input` ML-assisted resolution | — | — | — | **Missing**: real native ML runtime and model artifacts required; no mocked model inference or semantic-ranking parity claimed |
| `browser`, `worker`, `server`, `storage`, `storage/s3`, `storage/google-drive`, `server/auth`, `server/models`, `server/storage` | — | — | — | Platform-specific; portable data contracts may be ported, actual network/runtime adapters need separate native implementation |
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
