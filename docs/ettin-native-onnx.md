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


## Size-specific genuine-model verification expansion

The `ettin-real-sizes.yml` workflow now obtains the exact released `cross-encoder/ettin-reranker-{32,68,150}m-v1` model, tokenizer and all three scoring head files and executes the **same real native ONNX implementation** under Linux and macOS for every size. Independent immutable checkpoint revisions: 32M=`e3da5c5e7c2788d9e65351c380b19342d089800c`; 68M=`5ee8c7f9f0b951946db55ed4d60b0891539d5cb9`; 150M=`cfa56e0d46b3a6ad6a710afbc3f202a5f2290aae`. The job validates artifact presence and emits every file SHA-256 in its job summary. Exact revisions and real model execution must pass before these model sizes may be considered functionally integrated.

The model families remain **partially implemented** with respect to full source parity until the Hugging Face pair tokenizer and truncation semantics, float32 scores and original GutTS asynchronous API are differentially verified. The 17M model has already passed exact-main Linux/macOS inference; this new verification is not evidence of bitwise equivalence by itself.


## Exact original paired-tokenizer differential (2026-11-21)

`scripts/verify-ettin-tokenizer-source.mjs` and `tests/ettin_tokenizer_runner.cpp` compare real immutable `GutTS@3e2cbc0990ba29830ce91c5d1a1d552675d5334c` dependency `@huggingface/transformers@4.3.2` `AutoTokenizer.from_pretrained("cross-encoder/ettin-reranker-17m-v1", {revision:"3253335c59c0d70ababfdee87ad5390b4d1eff4d"})` against **actual native** `libembedding::TokenizerWrapper` plus `ettin_native_detail::pair_tokens`. It checks exact `input_ids`, `attention_mask` and available `token_type_ids` on realistic query/document pairs including multilingual text, combining marks and emoji. The separate `ettin-tokenizer-source.yml` CI requires **no tolerance** for token differences, and does not substitute offline hand-written fixture arrays for original model execution.

Genuine local CPU ONNX model execution remains independently validated for 17/32/68/150M; token parity evidence is separate. If this new source execution fails, `GutCPP` must not claim full pair-tokenizer compatibility or Wave 2 readiness. Numeric score comparison against real pinned original GutTS runtime is a subsequent distinct gate; passing tokenizer IDs alone is insufficient. Default dependency-free C++17 core is unchanged.


### Authentic Hugging Face tokenization repair (source-differential remediation)

The earlier lightweight `libembedding::TokenizerWrapper` did not reproduce the published Ettin byte-level pre-tokenizer/merge rules: the first genuine source test yielded 18 pinned Transformers.js token IDs versus 37 native IDs. The opt-in Ettin adapter now uses `mlc-ai/tokenizers-cpp@c586c52f93f7b060753bd2388eb96a105cb7374d`, which embeds Hugging Face's Rust `tokenizers@0.21.2` and loads the **original tokenizer.json** in-process. Raw text gets true byte-pair encoding with no special tokens, then the native code applies the model's `<s> query </s> document </s>` template. The source-executed CI gate compares actual token IDs/masks on diverse input pairs; this is a **proposed repair until its exact-head and exact-main checks pass**. The Rust tokenizer is opt-in for Ettin and is not required by the standalone dependency-free C++17 core.

This initial repair rejects token pairs exceeding 512 rather than silently approximating Hugging Face's `truncation=true` longest-first policy. Full equivalence therefore still requires tests and bounded implementation for long pairs, as well as numerical source-score differential. Zero silent fallback to the original simplified tokenizer is allowed.
