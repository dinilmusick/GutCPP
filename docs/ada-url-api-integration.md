# Optional registered WHATWG URL execution

The default portable C++17 target remains header-only and has a conservative
absolute URL parser. Production parity with GutTS \`new URL()\` and
\`URLSearchParams\` is available through the pinned Ada WHATWG backend,
now also wired to \`gut::execute_gutts_api\` for the three URL API IDs:

- \`gutts.urls.isUrl\`
- \`gutts.urls.getQueryParam\`
- \`gutts.urls.setQueryParam\`

Opt in **only** for CMake source builds:

\`\`\`sh
cmake -S . -B build-ada -DBUILD_TESTING=ON -DGUTCPP_TEST_ADA_URL=ON
cmake --build build-ada --target gutcpp_ada_api_runner gutcpp_ada_api_test
ctest --test-dir build-ada -R ada_api_test --output-on-failure
\`\`\`

Link \`gutcpp::ada_url\` in CMake to propagate both the pinned Ada library and
\`GUTCPP_WITH_ADA_URL=1\` for the actual registry dispatcher. This source-only
alias is deliberately not exported in installed binary/package metadata because
Ada is an external fetched build dependency; it cannot be silently assumed to
be available to unrelated consumers. Do **not** set \`GUTCPP_WITH_ADA_URL\`
without also linking the Ada library.

\`scripts/verify-ada-url.mjs\` executes the actual compiled immutable GutTS
URL operations for 174 URL/query cases. The golden workflow checks both the
standalone Ada implementation and the new registered API execution path.
The existing dedicated Ada URL workflow compiles and tests on Linux, macOS
and Windows. No claim is made for general URI normalization or the unrelated
\`gutts.urls.joinUrl\` function beyond its existing differential coverage.
