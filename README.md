# GutCPP

Portable, header-only C++17 implementations of applicable `GutTS` data and utility contracts, pinned to upstream `3e2cbc0990ba29830ce91c5d1a1d552675d5334c` for the Wave 1 parity inventory.

## Local build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The pinned GutTS golden regression in `testdata/upstream-golden/` is reproduced by GitHub CI and exercised against C++ implementations.

The project exports installable `gutcpp::gutcpp` CMake target; `consumer/` tests the installed package. See [upstream mapping and explicit missing features](docs/upstream-parity.md). This is an actively developed **partial implementation**, not a claim of full GutTS parity. Upstream JavaScript-compatible URL parsing, Unicode case transformations, runtime transport adapters, and ML services remain unimplemented.
