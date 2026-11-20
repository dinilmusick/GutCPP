# Optional ICU Unicode adapter (C++17)

include/gut/icu_strings.hpp adds ICU-backed Unicode casing, word tokenization, Latin accent deburring, grapheme-aware truncation, and fail-closed UTF-8 validation. The dependency-free portable core and existing gut/strings.hpp remain unchanged. Link icu-uc and icu-i18n explicitly via pkg-config or corresponding CMake ICU targets.

The pinned executable differential (scripts/verify-icu-strings.mjs) runs the compiled GutTS functions at commit 3e2cbc0990ba29830ce91c5d1a1d552675d5334c against the actual C++ ICU runner on identical string inputs. A dedicated workflow tests strict warnings, native Unicode, and installed-header consumers on Linux/macOS.

ICU word segmentation, Unicode-data versions, some Lodash Unicode regex rules, validator.js email acceptance, and UTF-16 lone-surrogate behavior can differ. This is an optional native engine, not proof of every JavaScript Unicode operation; never count untested cases or injected results as complete parity.
