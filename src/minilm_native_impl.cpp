// One translation unit owns the real libembedding native ONNX/tokenizer symbols.
// libembedding's C implementation headers are embedded inside C++17.  Older
// upstream source omits these prerequisite standard includes and uses the C11
// thread-local keyword; shim them without modifying the pinned dependency.
#include <sys/stat.h>
#include <climits>
#include <stdexcept>
#ifndef _Thread_local
#define _Thread_local thread_local
#endif
#define LIBEMBEDDING_IMPLEMENTATION
#ifndef LIBEMBEDDING_NO_DOWNLOAD
#define LIBEMBEDDING_NO_DOWNLOAD
#endif
#ifndef LIBEMBEDDING_NO_IMAGE
#define LIBEMBEDDING_NO_IMAGE
#endif
#include <libembedding/libembedding.h>
