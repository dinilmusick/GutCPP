# Optional ICU Unicode adapter (C++17)

include/gut/icu_strings.hpp adds ICU-backed Unicode casing, word tokenization, Latin accent deburring, grapheme-aware truncation, and fail-closed UTF-8 validation. The dependency-free portable core and existing gut/strings.hpp remain unchanged. Link icu-uc and icu-i18n explicitly via pkg-config or corresponding CMake ICU targets.

The pinned executable differential (scripts/verify-icu-strings.mjs) runs the compiled GutTS functions at commit 3e2cbc0990ba29830ce91c5d1a1d552675d5334c against the actual C++ ICU runner on identical string inputs. A dedicated workflow tests strict warnings, native Unicode, and installed-header consumers on Linux/macOS.

ICU word segmentation, Unicode-data versions, some Lodash Unicode regex rules, validator.js email acceptance, and UTF-16 lone-surrogate behavior can differ. This is an optional native engine, not proof of every JavaScript Unicode operation; never count untested cases or injected results as complete parity.


### Differential remediation

The first executed pinned GutTS comparison matched 173/200 and exposed actual
emoji/ZWJ/flag grapheme omissions, letter-numeral splits and Greek combining
mark boundaries. The native word tokenizer now groups ICU character-cluster
units and preserves emoji runs, Unicode letter/digit boundaries and non-Latin
combining marks. Regression cases are retained in the upstream comparison, not
suppressed. ATAX obstacle: 8850b5bb-6f6e-43ea-9fa0-b90ec741484d.


### Registry API opt-in integration

The plain `gutcpp::gutcpp` C++17 header target intentionally remains ICU-free and maintains the historic ASCII-only dispatch. For Unicode support, configure with `-DGUTCPP_ENABLE_ICU_STRINGS=ON` and link the exported `gutcpp::icu` interface target. This target adds `GUTCPP_WITH_ICU_STRINGS=1` and ICU `uc`/`i18n`; the canonical `gut::execute_gutts_api` routes `capitalize`, `camelCase`, `kebabCase`, `snakeCase`, and `truncate` to real ICU operations. The `truncateWords` operation remains separate and uses existing portable behavior. The existing 200-case pinned compiled GutTS Unicode differential now also runs against the registry dispatcher. A real installed consumer calls the API, not just standalone helpers. The target and package config are exported only if the build enabled ICU.
