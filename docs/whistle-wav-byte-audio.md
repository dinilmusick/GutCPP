# Whistle WAV byte input and 16 kHz mono conversion

For the real Cactus 4-backed `gut::WhistleNativeSession` (opt-in `gutcpp::whistle`),
use `model.transcribe_wav(bytes, language)` or
`model.transcribe_detailed_wav(bytes, language, word_timestamps)` to send an
uncompressed WAV file stored in `std::vector<std::uint8_t>` directly to the
native speech runtime. These methods actually invoke `cactus_transcribe` after
decoding; there is no simulated transcript or fallback text.

The portable `gut::decode_wav_to_16khz` helper is available without the model
engine or an additional dependency and supports little-endian RIFF/WAVE:
integer PCM at 8, 16, 24, or 32 bits; IEEE float at 32 or 64 bits; and
WAVE_FORMAT_EXTENSIBLE when its valid-bit count matches the storage width.
Sample rates 8 kHz through 96 kHz and up to 8 channels are mixed down evenly
and resampled to 16 kHz mono float32. Resampling is **linear interpolation**;
it is deterministic but not a high-quality bandlimited downsampler. The maximum
recording length is 600 seconds.

Unsupported/compressed files, truncated RIFF chunks, inconsistent byte rates,
invalid data length, NaN/infinite/out-of-range samples, excess duration,
noncanonical extensible GUIDs, and multi-data layouts throw
`std::invalid_argument`. No decoder silently treats arbitrary bytes as PCM.

`tests/wav_pcm_test.cpp` runs as a default C++17 CTest on Linux, macOS and
Windows; its negative corpus, valid format matrix, and 800 deterministic
mutation cases also run under the existing Linux AddressSanitizer and
UndefinedBehaviorSanitizer matrix. `tests/whistle_native_test.cpp` separately
feeds actual WAV silence and generated audio through the checksum-verified
Whistle model in native Linux/macOS GitHub CI.

This closes a useful GutTS `Uint8Array` audio path but is not exhaustive audio
parity: MP3/FLAC/OGG, RF64/AIFF, exotic WAV codecs, exact browser audio
resampling fidelity, and Windows native Cactus speech execution remain unverified.
