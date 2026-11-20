// One translation unit owns the real libembedding native ONNX/tokenizer symbols.
// This model backend is optional and never compiled into gutcpp::gutcpp.
#define LIBEMBEDDING_IMPLEMENTATION
#define LIBEMBEDDING_NO_DOWNLOAD
#define LIBEMBEDDING_NO_IMAGE
#include <libembedding/libembedding.h>
