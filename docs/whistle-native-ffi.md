# Whistle real speech model through Cactus 4 (C++17)

GutCPP's optional `gutcpp::whistle` CMake target runs the **real** Whistle
speech-to-text model through the official Cactus 4 native C ABI. It links a
verified prebuilt engine and loads an immutable ".cact" checkpoint; neither
weights nor any binaries are included in the GutCPP source tree. The default
`gutcpp::gutcpp` C++17 target remains header-only, without the speech engine.

## Immutable provenance and hashes

The pinned source-of-truth downloader is Apache-2.0
`cactus-compute/cactus@bb822a35983cf0ff942d67d46b8b79b535773b02`.
Its `cactus.agent.fetch` module checks all artifact hashes before returning:

- Cactus engine release **4.1.4**, revision
  `f4ab2741203a282c6e7b89983de2b5e5b2e805a8`, with SHA-256 release
  manifest `ab948753f08e54d2b24dd04830dcb984885524ab3bd1e9da3bf207276ab62eb6`.
- `Cactus-Compute/whistle` model revision
  `88cb00eeb8928ddbcd410b491584f938ba030b07`, with `.cact` model SHA-256
  `b6e02f048568ac5d01a2042556c658061e699acbc0aa2a1439f52f3d461dffeb`.
- Pinned GutTS reference:
  `3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.

The GitHub CI checks out the pinned downloader, installs it inside a Python
virtual environment (so macOS PEP 668 is respected), downloads the engine and
model, validates all hashes, and links the real C++ binary against `libcactus`.

`#include <gut/whistle_native.hpp>` offers a `WhistleNativeSession(path)` for
real model loading and `transcribe_detailed(samples, language, timestamps)`
returning validated native `{text,language,ttft_ms,decode_tps,...}` data.
`transcribe(samples,language)` returns the transcription text as GutTS expects.
Inputs are 16 kHz mono float32 PCM with samples in `[-1,1]`; the caller handles
WAV decoding and resampling. Supported forced languages: en, de, fr, es, it,
nl, pl. Invalid model paths, negative/NaN amplitudes, empty input and unknown
languages are rejected, not silently processed.

**Important engine ownership:** the official Cactus C API stores a
**process-global model**, not an unloadable per-session handle. GutCPP serializes
its own loads and transcription, and reloads the correct file when a different
GutCPP Whistle session is used. Do not mix GutCPP native sessions with other
code directly loading a different model in the same libcactus process without
external coordination. No destructor pretends to unload weights.

## Verification / remaining gap

The Linux/macOS real-model integration tests use one second of silence and
three seconds of non-silent waveforms. They assert valid model outputs and the
forced language path, including timed words, against the real engine, not a
mock. These cases verify actual native transcription paths, but are **not**
a speech quality benchmark; broad human speech transcription, streaming,
WAV decoding/resampling, Windows native linkage and all other model runtimes
remain distinct gaps toward complete GutTS behavioral parity.
