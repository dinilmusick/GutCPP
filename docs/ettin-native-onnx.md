# Native Ettin reranker: ONNX tokenizer, transformer backbone and Safetensors head

This opt-in C++17 implementation performs **actual local model inference** rather than
calling a placeholder callback, a remote service or a scoring head fed with synthetic
embeddings. It is deliberately separate from dependency-free `gutcpp::gutcpp`.

## Source and artifact provenance

- Pinned contract: `dinilmusick/GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c`.
- Reused native ONNX and tokenizer engine: MIT `pacifio/libembedding@d8d4e7d7769a0b67fa093a62730110e91253bb87`.
- CPU execution engine: system-installed `onnxruntime` development library, version reported by CI.
- Initial complete real inference proof: `cross-encoder/ettin-reranker-17m-v1@3253335c59c0d70ababfdee87ad5390b4d1eff4d`.
- The required real artifacts are `onnx/model.onnx` (expected SHA-256
  `2a375ccea150a4df925a47884b510eeb4bd6227ca6fffdbbc62ca2284bfe6211`),
  `tokenizer.json`, `2_Dense/model.safetensors`,
  `3_LayerNorm/model.safetensors` and `4_Dense/model.safetensors`.

## Compile and test

```sh
cmake -S . -B build-ettin -DBUILD_TESTING=ON -DGUTCPP_ENABLE_ETTIN_ONNX=ON \
  -DONNXRUNTIME_ROOT=/path/to/onnxruntime \
  -DGUTCPP_ETTIN_MODEL_PATH=/path/to/real/model-dir
cmake --build build-ettin --target gutcpp_ettin_native_test --parallel 2
ctest --test-dir build-ettin -R ettin_actual_model_reranking --output-on-failure
```

`gut::EttinNativeSession(artifact_directory)` loads the real Hugging Face
WordPiece/BPE tokenizer, ONNX transformer (requires a real `last_hidden_state`),
and Safetensors dense, normalization and classification head. For each
query/document pair, CPU inference feeds real `input_ids`, `attention_mask`
and optional `token_type_ids` to the backbone; the pinned float32 head
produces a finite logistic relevance score. `rerank(query, candidates)`
returns scores **in input order**, as required by the original GutTS contract.
It does not silently sort them. A move-only RAII session owns native resources.

The continuous-integration workflow checks actual model loading and inference
on Linux and macOS, deterministic repeated scores, bounded inputs, invalid
model paths and correct move/close behavior.

## Important behavioral limits

The pinned `libembedding` tokenizer uses a simplified WordPiece/BPE
implementation and does not replicate every Hugging Face tokenizer
normalizer, pair-template, Unicode or truncation rule. This native adapter
currently constructs BERT-style `[CLS] query [SEP] document [SEP]`
sequences explicitly, with token-type IDs. The exact token IDs and scores
therefore require additional source-pinned differential verification against
the real GutTS Transformers.js Ettin pipeline. No claim of bitwise logits,
unbounded semantic parity, JavaScript Promise/AbortSignal behavior, or
Unicode equivalence is justified. The native model must fail if the
artifact, ONNX output or tokenizer structure is unavailable.

This code is parametrized by the model directory, but only the 17M weights
are verified in the initial real-model job. Authentic 32M/68M/150M models
must each pass their own pinned revision, real head/backbone and differential
tests before all size variants can be marked verified.

Wave 2 remains blocked until all applicable portable contracts and
independent ATAX verifications pass.
