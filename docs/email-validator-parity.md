# GutTS default `isEmail` native C++17 parity

Pinned JS source: `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c` → `src/strings/string-predicates.ts` → `validator` v13.15.15 `isEmail(value)`.

`gut::is_email` implements the **default** validator.js behavior without a Node.js dependency: local part UTF-8 byte max 64, domain UTF-8 byte max 254, total ECMAScript UTF-16 unit max 254, quoted local strings and backslash escapes, UTF-8/BMP local characters, validator.js FQDN label and top-level-domain rules, fullwidth prohibition, and hyphen/underscore exclusions. Malformed UTF-8 fails closed. Unsupported non-default options (`allow_ip_domain`, `allow_display_name`, optional host lists, `domain_specific_validation`, and others) are not promised; GutTS never passes these options through its public wrapper.

The checked-in cross-language regression builds the **actual immutable GutTS** package, calls its exported `isEmail` wrapper in Node.js, and compares thousands of executable native C++17 outcomes. The core remains a dependency-free header-only C++17 library.

**Not full library parity:** this implementation closes the default `isEmail` surface but does not imply a native execution backend for GutTS model inference, exact JS object property order, all Unicode word patterns, or live cloud provider certification.
