# Real MiniLM ONNX model inference in GutCPP

Source pin: `GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.
Real native MiniLM backbone and tokenizer use the MIT-licensed `pacifio/libembedding@d8d4e7d7769a0b67fa093a62730110e91253bb87`, built against ONNX Runtime 1.21.0. The model and tokenizer are the actual `sentence-transformers/all-MiniLM-L6-v2` ONNX and tokenizer files from Hugging Face commit `1110a243fdf4706b3f48f1d95db1a4f5529b4d41`. No fake embeddings or precomputed response fixtures.

Compile only if opted in:

```
cmake -S . -B build-minilm -DBUILD_TESTING=ON -DGUTCPP_ENABLE_MINILM_ONNX=ON -DONNXRUNTIME_ROOT=/path/to/onnxruntime
cmake --build build-minilm --target gutcpp_minilm_native_test gutcpp_minilm_native_runner
```

Link `gutcpp::minilm_native` to use `gut::MiniLMNativeSession`, which owns real ONNX model weights and tokenizer and calls the native tokenizer, ONNX backbone, attention-mask mean pooling, float32 L2 normalization and existing pinned GutTS numeric cosine logic. A session supports `embed`, `embed_batch`, `similarity`, `loaded`, `close`, move ownership, and validates model and text input. The ordinary `gutcpp::gutcpp` header-only target does not gain dependencies.

`minilm-real-inference.yml` compiles and loads actual artifacts on Linux and macOS and compares numerical cosine similarity against the **real compiled pinned GutTS MiniLM** runtime on 12 paired prompts. Tolerance is 0.025 because the ONNX model revisions, pooling edge cases and the two independent inference runtimes can differ at lower levels; a passing tolerance is **not** proof of bit identity. The one-time artifact download is CI-only; runtime loads local weights supplied by the caller. Model checks must pass before merge.
