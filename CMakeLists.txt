cmake_minimum_required(VERSION 3.16)
project(GutCPP VERSION 0.1.0 LANGUAGES CXX)
include(CTest)
include(GNUInstallDirs)
add_library(gutcpp INTERFACE)
add_library(gutcpp::gutcpp ALIAS gutcpp)
target_compile_features(gutcpp INTERFACE cxx_std_17)
target_include_directories(gutcpp INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# Optional Unicode-capable API dispatcher. The default gutcpp::gutcpp target
# remains entirely ICU-free; the opt-in target propagates its compile flag.
option(GUTCPP_ENABLE_ICU_STRINGS "Export an ICU-backed GutTS API dispatcher" OFF)
if(GUTCPP_ENABLE_ICU_STRINGS)
  find_package(ICU REQUIRED COMPONENTS uc i18n)
  add_library(gutcpp_icu INTERFACE)
  set_target_properties(gutcpp_icu PROPERTIES EXPORT_NAME icu)
  add_library(gutcpp::icu ALIAS gutcpp_icu)
  target_link_libraries(gutcpp_icu INTERFACE gutcpp::gutcpp ICU::uc ICU::i18n)
  target_compile_definitions(gutcpp_icu INTERFACE GUTCPP_WITH_ICU_STRINGS=1)
  if(BUILD_TESTING)
    foreach(target IN ITEMS icu_api_test icu_api_runner)
      add_executable(gutcpp_${target} tests/${target}.cpp)
      target_link_libraries(gutcpp_${target} PRIVATE gutcpp::icu)
      if(MSVC)
        target_compile_options(gutcpp_${target} PRIVATE /W4 /WX /permissive- /utf-8)
      else()
        target_compile_options(gutcpp_${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
      endif()
    endforeach()
    add_test(NAME icu_api_test COMMAND gutcpp_icu_api_test)
  endif()
endif()
if(BUILD_TESTING)
  set(GUTCPP_TEST_FILES
    tests/minilm_math_test.cpp
    tests/google_drive_test.cpp
    tests/luth_client_test.cpp
    tests/model_library_test.cpp
    tests/server_auth_test.cpp
    tests/application_auth_client_test.cpp
    tests/luth_types_test.cpp
    tests/model_registry_test.cpp
    tests/semantic_input_test.cpp
    tests/semantic_workflow_test.cpp
    tests/ettin_head_test.cpp
    tests/engineering_test.cpp
    tests/model_residency_test.cpp
    tests/api_test.cpp
    tests/strings_test.cpp
    tests/wav_pcm_test.cpp
    tests/email_validator_test.cpp
    tests/portable_test.cpp
    tests/json_test.cpp
    tests/json_ecmascript_test.cpp
    tests/net_test.cpp
    tests/auth_contracts_test.cpp
    tests/result_test.cpp
    tests/url_test.cpp
    tests/percent_encoding_test.cpp
    tests/storage_path_test.cpp
    tests/storage_path_properties_test.cpp
    tests/upstream_golden_test.cpp)
  find_package(Threads REQUIRED)
  foreach(test_file IN LISTS GUTCPP_TEST_FILES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_file}" AND
      (NOT test_file STREQUAL "tests/upstream_golden_test.cpp" OR
        (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/testdata/upstream-golden/gutts-3e2cbc0.json" AND
         EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/gut/storage_path.hpp")))
      get_filename_component(test_name "${test_file}" NAME_WE)
      add_executable("gutcpp_${test_name}" "${test_file}")
      target_link_libraries("gutcpp_${test_name}" PRIVATE gutcpp::gutcpp Threads::Threads)
      if(MSVC)
        target_compile_options("gutcpp_${test_name}" PRIVATE /W4 /WX /permissive- /utf-8)
      else()
        target_compile_options("gutcpp_${test_name}" PRIVATE -Wall -Wextra -Wpedantic -Werror)
      endif()
      if(test_file STREQUAL "tests/upstream_golden_test.cpp")
        target_compile_definitions("gutcpp_${test_name}" PRIVATE
          GUTCPP_GOLDEN_PATH="${CMAKE_CURRENT_SOURCE_DIR}/testdata/upstream-golden/gutts-3e2cbc0.json")
      endif()

      add_test(NAME "${test_name}" COMMAND "gutcpp_${test_name}")
      if(test_file STREQUAL "tests/model_library_test.cpp")
        target_compile_definitions("gutcpp_${test_name}" PRIVATE
          GUTCPP_MODEL_LIBRARY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/README.md")
      endif()
      if(test_file STREQUAL "tests/server_auth_test.cpp")
        set_tests_properties("${test_name}" PROPERTIES ENVIRONMENT "APP_URL=  https://example.com/app  ")
      endif()
    endif()
  endforeach()
endif()

# Opt-in real HTTP tests; the portable header-only target stays dependency-free.
option(GUTCPP_TEST_CURL "Exercise native libcurl transport against loopback HTTP" OFF)
if(BUILD_TESTING AND GUTCPP_TEST_CURL)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GUTCPP_LIBCURL REQUIRED IMPORTED_TARGET libcurl)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  foreach(http_test IN ITEMS curl_loopback luth_curl_integration google_drive_curl)
    add_executable("gutcpp_${http_test}_test" "tests/${http_test}_test.cpp")
    target_link_libraries("gutcpp_${http_test}_test" PRIVATE gutcpp::gutcpp PkgConfig::GUTCPP_LIBCURL Threads::Threads)
    if(MSVC)
      target_compile_options("gutcpp_${http_test}_test" PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options("gutcpp_${http_test}_test" PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if(http_test STREQUAL "google_drive_curl")
      set(GUTCPP_HTTP_SERVER "${CMAKE_CURRENT_SOURCE_DIR}/tests/google_drive_loopback_server.py")
    else()
      set(GUTCPP_HTTP_SERVER "${CMAKE_CURRENT_SOURCE_DIR}/tests/curl_loopback_server.py")
    endif()
    add_test(NAME "${http_test}_test" COMMAND "${Python3_EXECUTABLE}"
      "${GUTCPP_HTTP_SERVER}" "$<TARGET_FILE:gutcpp_${http_test}_test>")
  endforeach()
endif()


# Opt-in actual AWS S3 Signature Version 4 HTTP and crypto verification.
# Neither OpenSSL nor libcurl is needed by the portable target.
option(GUTCPP_TEST_S3 "Execute signed native S3 interoperability tests" OFF)
if(BUILD_TESTING AND GUTCPP_TEST_S3)
  find_package(OpenSSL REQUIRED COMPONENTS Crypto)
  find_package(PkgConfig REQUIRED)
  if(NOT TARGET PkgConfig::GUTCPP_LIBCURL)
    pkg_check_modules(GUTCPP_LIBCURL REQUIRED IMPORTED_TARGET libcurl)
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_executable(gutcpp_s3_sigv4_test tests/s3_sigv4_test.cpp)
  target_link_libraries(gutcpp_s3_sigv4_test PRIVATE gutcpp::gutcpp OpenSSL::Crypto Threads::Threads)
  add_executable(gutcpp_s3_curl_test tests/s3_curl_test.cpp)
  target_link_libraries(gutcpp_s3_curl_test PRIVATE gutcpp::gutcpp OpenSSL::Crypto PkgConfig::GUTCPP_LIBCURL Threads::Threads)
  foreach(s3_test IN ITEMS gutcpp_s3_sigv4_test gutcpp_s3_curl_test)
    if(MSVC)
      target_compile_options(${s3_test} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(${s3_test} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()
  add_test(NAME s3_sigv4_test COMMAND gutcpp_s3_sigv4_test)
  add_test(NAME s3_curl_test COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/s3_loopback_server.py"
    "$<TARGET_FILE:gutcpp_s3_curl_test>")
endif()


# Optional WHATWG URL compatibility backed by immutable ada-url 2.6.10 source.
# Ada is NOT a dependency of the default portable header-only library.
option(GUTCPP_TEST_ADA_URL "Compile real WHATWG URL compatibility against pinned Ada" OFF)
if(GUTCPP_TEST_ADA_URL)
  include(FetchContent)
  set(GUTCPP_PARENT_BUILD_TESTING "${BUILD_TESTING}")
  set(BUILD_TESTING OFF)
  set(ADA_TOOLS OFF CACHE BOOL "Disable Ada CLI tooling and unrelated fmt/cxxopts transitive dependencies" FORCE)
  set(ADA_BENCHMARKS OFF CACHE BOOL "Do not compile Ada benchmarks" FORCE)
  FetchContent_Declare(gutcpp_ada
    GIT_REPOSITORY https://github.com/ada-url/ada.git
    GIT_TAG 812b97865c000325ff6b1cac48e89530a919fa7b
    GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(gutcpp_ada)
  set(BUILD_TESTING "${GUTCPP_PARENT_BUILD_TESTING}")

  # Source-build-only opt-in target: the Ada library is fetched at an immutable
  # revision and is not silently made a dependency of installed portable GutCPP.
  add_library(gutcpp_ada_api INTERFACE)
  add_library(gutcpp::ada_url ALIAS gutcpp_ada_api)
  target_link_libraries(gutcpp_ada_api INTERFACE gutcpp::gutcpp ada::ada)
  target_compile_definitions(gutcpp_ada_api INTERFACE GUTCPP_WITH_ADA_URL=1)
  foreach(url_target IN ITEMS gutcpp_ada_url_test gutcpp_ada_url_runner)
    if(url_target STREQUAL "gutcpp_ada_url_test")
      add_executable(${url_target} tests/ada_url_test.cpp)
    else()
      add_executable(${url_target} tests/ada_url_runner.cpp)
    endif()
    target_link_libraries(${url_target} PRIVATE gutcpp::gutcpp ada::ada)
    if(MSVC)
      target_compile_options(${url_target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(${url_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()
  foreach(api_target IN ITEMS gutcpp_ada_api_test gutcpp_ada_api_runner)
    if(api_target STREQUAL "gutcpp_ada_api_test")
      add_executable(${api_target} tests/ada_api_test.cpp)
    else()
      add_executable(${api_target} tests/ada_api_runner.cpp)
    endif()
    target_link_libraries(${api_target} PRIVATE gutcpp::ada_url)
    if(MSVC)
      target_compile_options(${api_target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(${api_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()
  if(BUILD_TESTING)
    add_test(NAME ada_url_test COMMAND gutcpp_ada_url_test)
    add_test(NAME ada_api_test COMMAND gutcpp_ada_api_test)
  endif()
endif()

# Optional executable JSON-Lines adapter for differential tests against the
# immutable compiled GutTS reference. No Node.js dependency in the library.
option(GUTCPP_DIFFERENTIAL_RUNNER "Build JSON-Lines native API differential driver" OFF)
if(GUTCPP_DIFFERENTIAL_RUNNER)
  add_executable(gutcpp_email_validator_runner tests/email_validator_runner.cpp)
  target_link_libraries(gutcpp_email_validator_runner PRIVATE gutcpp::gutcpp)
  if(MSVC)
    target_compile_options(gutcpp_email_validator_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_email_validator_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(gutcpp_json_number_runner tests/json_number_runner.cpp)
  target_link_libraries(gutcpp_json_number_runner PRIVATE gutcpp::gutcpp)
  if(MSVC)
    target_compile_options(gutcpp_json_number_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_json_number_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(gutcpp_minilm_math_runner tests/minilm_math_runner.cpp)
  target_link_libraries(gutcpp_minilm_math_runner PRIVATE gutcpp::gutcpp)
  if(MSVC)
    target_compile_options(gutcpp_minilm_math_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_minilm_math_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(gutcpp_ettin_head_runner tests/ettin_head_runner.cpp)
  target_link_libraries(gutcpp_ettin_head_runner PRIVATE gutcpp::gutcpp)
  add_executable(gutcpp_storage_path_differential_runner tests/storage_path_differential_runner.cpp)
  target_link_libraries(gutcpp_storage_path_differential_runner PRIVATE gutcpp::gutcpp)
  if(MSVC)
    target_compile_options(gutcpp_storage_path_differential_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_storage_path_differential_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(gutcpp_differential_runner tests/api_differential_runner.cpp)
  target_link_libraries(gutcpp_differential_runner PRIVATE gutcpp::gutcpp)
  if(MSVC)
    target_compile_options(gutcpp_ettin_head_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_ettin_head_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  if(MSVC)
    target_compile_options(gutcpp_differential_runner PRIVATE /W4 /WX /permissive- /utf-8)
  else()
    target_compile_options(gutcpp_differential_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endif()

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(TARGETS gutcpp EXPORT GutCPPTargets)
if(GUTCPP_ENABLE_ICU_STRINGS)
  install(TARGETS gutcpp_icu EXPORT GutCPPTargets)
endif()
install(EXPORT GutCPPTargets NAMESPACE gutcpp:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")
include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GutCPPConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfig.cmake"
 "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")


# Explicit opt-in to a REAL Needle3 v3 native model runtime. The default core
# remains header-only and has no Rust, model-weight or C ABI dependency.
option(GUTCPP_ENABLE_NEEDLE3 "Enable real Needle3 v3 inference via pinned native C ABI" OFF)
if(GUTCPP_ENABLE_NEEDLE3)
  if(NOT GUTCPP_NEEDLE_C_LIBRARY)
    find_library(GUTCPP_NEEDLE_C_LIBRARY NAMES needle_c)
  endif()
  if(NOT GUTCPP_NEEDLE_C_LIBRARY OR NOT EXISTS "${GUTCPP_NEEDLE_C_LIBRARY}")
    message(FATAL_ERROR "Set GUTCPP_NEEDLE_C_LIBRARY to actual libneedle_c built from pinned needle-rs")
  endif()
  add_library(gutcpp_native_needle UNKNOWN IMPORTED)
  set_target_properties(gutcpp_native_needle PROPERTIES IMPORTED_LOCATION "${GUTCPP_NEEDLE_C_LIBRARY}")
  add_library(gutcpp_needle3 INTERFACE)
  add_library(gutcpp::needle3 ALIAS gutcpp_needle3)
  target_link_libraries(gutcpp_needle3 INTERFACE gutcpp::gutcpp gutcpp_native_needle)
  if(BUILD_TESTING)
    if(NOT GUTCPP_NEEDLE3_MODEL_PATH OR NOT EXISTS "${GUTCPP_NEEDLE3_MODEL_PATH}")
      message(FATAL_ERROR "Set GUTCPP_NEEDLE3_MODEL_PATH to the real Needle3 .cact checkpoint")
    endif()
    add_executable(gutcpp_needle3_native_test tests/needle3_native_test.cpp)
    target_link_libraries(gutcpp_needle3_native_test PRIVATE gutcpp::needle3)
    if(MSVC)
      target_compile_options(gutcpp_needle3_native_test PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(gutcpp_needle3_native_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME needle3_real_inference COMMAND gutcpp_needle3_native_test "${GUTCPP_NEEDLE3_MODEL_PATH}")
    set_tests_properties(needle3_real_inference PROPERTIES TIMEOUT 300)
  endif()
endif()


# Opt-in Cactus 4 speech inference: externally supplied, checksum-verified
# libcactus, never a dependency of gutcpp::gutcpp or an installed package.
option(GUTCPP_ENABLE_WHISTLE "Enable Whistle speech recognition via real native Cactus C ABI" OFF)
if(GUTCPP_ENABLE_WHISTLE)
  if(NOT GUTCPP_CACTUS_LIBRARY)
    find_library(GUTCPP_CACTUS_LIBRARY NAMES cactus libcactus)
  endif()
  if(NOT GUTCPP_CACTUS_LIBRARY OR NOT EXISTS "${GUTCPP_CACTUS_LIBRARY}")
    message(FATAL_ERROR "Set GUTCPP_CACTUS_LIBRARY to actual pinned libcactus native shared library")
  endif()
  add_library(gutcpp_native_cactus UNKNOWN IMPORTED)
  set_target_properties(gutcpp_native_cactus PROPERTIES IMPORTED_LOCATION "${GUTCPP_CACTUS_LIBRARY}")
  add_library(gutcpp_whistle INTERFACE)
  add_library(gutcpp::whistle ALIAS gutcpp_whistle)
  target_link_libraries(gutcpp_whistle INTERFACE gutcpp::gutcpp gutcpp_native_cactus)
  if(BUILD_TESTING)
    if(NOT GUTCPP_WHISTLE_MODEL_PATH OR NOT EXISTS "${GUTCPP_WHISTLE_MODEL_PATH}")
      message(FATAL_ERROR "Set GUTCPP_WHISTLE_MODEL_PATH to checksum-verified whistle.cact weights")
    endif()
    find_package(Threads REQUIRED)
    add_executable(gutcpp_whistle_native_test tests/whistle_native_test.cpp)
    target_link_libraries(gutcpp_whistle_native_test PRIVATE gutcpp::whistle Threads::Threads)
    if(MSVC)
      target_compile_options(gutcpp_whistle_native_test PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(gutcpp_whistle_native_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME whistle_real_transcription COMMAND gutcpp_whistle_native_test "${GUTCPP_WHISTLE_MODEL_PATH}")
    set_tests_properties(whistle_real_transcription PROPERTIES TIMEOUT 300)
  endif()
endif()


# Opt-in actual MiniLM transformer execution, using pinned MIT libembedding and
# a user-supplied ONNX Runtime installation. Not a default GutCPP dependency.
option(GUTCPP_ENABLE_MINILM_ONNX "Build real native MiniLM-L6-v2 ONNX model runtime" OFF)
if(GUTCPP_ENABLE_MINILM_ONNX)
  include(FetchContent)
  set(LIBEMBEDDING_NO_DOWNLOAD ON CACHE BOOL "Caller supplies pinned ONNX model" FORCE)
  set(LIBEMBEDDING_NO_IMAGE ON CACHE BOOL "Text embeddings only" FORCE)
  set(LIBEMBEDDING_BUILD_TESTS OFF CACHE BOOL "Skip unrelated libembedding tests" FORCE)
  set(LIBEMBEDDING_BUILD_EXAMPLES OFF CACHE BOOL "Skip unrelated libembedding examples" FORCE)
  set(LIBEMBEDDING_BUILD_BENCHMARKS OFF CACHE BOOL "Skip unrelated libembedding benchmarks" FORCE)
  set(LIBEMBEDDING_BUILD_SHARED OFF CACHE BOOL "Source-only implementation" FORCE)
  FetchContent_Declare(gutcpp_libembedding
    GIT_REPOSITORY https://github.com/pacifio/libembedding.git
    GIT_TAG d8d4e7d7769a0b67fa093a62730110e91253bb87
    GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(gutcpp_libembedding)
  add_library(gutcpp_minilm_native STATIC src/minilm_native_impl.cpp)
  add_library(gutcpp::minilm_native ALIAS gutcpp_minilm_native)
  target_link_libraries(gutcpp_minilm_native PUBLIC gutcpp::gutcpp libembedding::libembedding)
  target_compile_features(gutcpp_minilm_native PUBLIC cxx_std_17)
  foreach(target IN ITEMS gutcpp_minilm_native_test gutcpp_minilm_native_runner)
    if(target STREQUAL "gutcpp_minilm_native_test")
      add_executable(${target} tests/minilm_native_test.cpp)
    else()
      add_executable(${target} tests/minilm_native_runner.cpp)
    endif()
    target_link_libraries(${target} PRIVATE gutcpp::minilm_native)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()
endif()


# Optional genuine SmolLM2-135M autoregressive generation using pinned GGUF.
option(GUTCPP_ENABLE_SMOLLM2_GGUF "Build actual native SmolLM2-135M llama.cpp runtime" OFF)
if(GUTCPP_ENABLE_SMOLLM2_GGUF)
  include(FetchContent)
  set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable unrelated llama tests" FORCE)
  set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama examples" FORCE)
  set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama server" FORCE)
  set(GGML_NATIVE OFF CACHE BOOL "Portable CPU builds" FORCE)
  set(GGML_METAL OFF CACHE BOOL "CPU-only generation" FORCE)
  set(GGML_CUDA OFF CACHE BOOL "CPU-only generation" FORCE)
  FetchContent_Declare(gutcpp_llama_cpp
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG 40b7a546defe1f60edd1338c719552ffc58c89a0
    GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(gutcpp_llama_cpp)
  add_library(gutcpp_smollm2_native INTERFACE)
  add_library(gutcpp::smollm2_native ALIAS gutcpp_smollm2_native)
  target_link_libraries(gutcpp_smollm2_native INTERFACE gutcpp::gutcpp llama)
  target_compile_features(gutcpp_smollm2_native INTERFACE cxx_std_17)
  if(BUILD_TESTING)
    add_executable(gutcpp_smollm2_native_test tests/smollm2_native_test.cpp)
    target_link_libraries(gutcpp_smollm2_native_test PRIVATE gutcpp::smollm2_native)
    if(MSVC)
      target_compile_options(gutcpp_smollm2_native_test PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(gutcpp_smollm2_native_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if(GUTCPP_SMOLLM2_MODEL_PATH)
      add_test(NAME smollm2_real_generation COMMAND gutcpp_smollm2_native_test "${GUTCPP_SMOLLM2_MODEL_PATH}")
      set_tests_properties(smollm2_real_generation PROPERTIES TIMEOUT 240)
    endif()
  endif()
endif()


# Genuine opt-in Ettin cross-encoder model inference (actual tokenizer, ONNX
# transformer and pinned Safetensors scoring head). Default core stays clean.
option(GUTCPP_ENABLE_ETTIN_ONNX "Enable genuine native Ettin ONNX transformer reranking" OFF)
if(GUTCPP_ENABLE_ETTIN_ONNX)
  include(FetchContent)
  set(LIBEMBEDDING_NO_DOWNLOAD ON CACHE BOOL "Ettin uses caller-supplied model" FORCE)
  set(LIBEMBEDDING_NO_IMAGE ON CACHE BOOL "Ettin text-only model" FORCE)
  set(LIBEMBEDDING_BUILD_TESTS OFF CACHE BOOL "Skip unrelated libembedding tests" FORCE)
  set(LIBEMBEDDING_BUILD_EXAMPLES OFF CACHE BOOL "Skip unrelated libembedding examples" FORCE)
  set(LIBEMBEDDING_BUILD_BENCHMARKS OFF CACHE BOOL "Skip unrelated libembedding benchmarks" FORCE)
  set(LIBEMBEDDING_BUILD_SHARED OFF CACHE BOOL "Native in-process model" FORCE)
  if(NOT TARGET libembedding::libembedding)
    FetchContent_Declare(gutcpp_libembedding
      GIT_REPOSITORY https://github.com/pacifio/libembedding.git
      GIT_TAG d8d4e7d7769a0b67fa093a62730110e91253bb87
      GIT_SHALLOW FALSE)
    FetchContent_MakeAvailable(gutcpp_libembedding)
  endif()
  # Authenticated Hugging Face tokenizer JSON is parsed with real Rust
  # tokenizers 0.21.x rather than simplified libembedding byte-level BPE.
  set(MLC_ENABLE_SENTENCEPIECE_TOKENIZER OFF CACHE BOOL "Ettin only needs HF tokenizer" FORCE)
  FetchContent_Declare(gutcpp_hf_tokenizers
    GIT_REPOSITORY https://github.com/mlc-ai/tokenizers-cpp.git
    GIT_TAG c586c52f93f7b060753bd2388eb96a105cb7374d
    GIT_SHALLOW FALSE
    GIT_SUBMODULES_RECURSE TRUE)
  FetchContent_MakeAvailable(gutcpp_hf_tokenizers)
  add_library(gutcpp_ettin_native INTERFACE)
  add_library(gutcpp::ettin_native ALIAS gutcpp_ettin_native)
  target_link_libraries(gutcpp_ettin_native INTERFACE gutcpp::gutcpp libembedding::libembedding tokenizers_cpp)
  target_compile_features(gutcpp_ettin_native INTERFACE cxx_std_17)
  if(BUILD_TESTING)
    add_executable(gutcpp_ettin_tokenizer_runner tests/ettin_tokenizer_runner.cpp)
    target_link_libraries(gutcpp_ettin_tokenizer_runner PRIVATE gutcpp::ettin_native)
    if(MSVC)
      target_compile_options(gutcpp_ettin_tokenizer_runner PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(gutcpp_ettin_tokenizer_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_executable(gutcpp_ettin_native_test tests/ettin_native_test.cpp)
    target_link_libraries(gutcpp_ettin_native_test PRIVATE gutcpp::ettin_native)
    if(MSVC)
      target_compile_options(gutcpp_ettin_native_test PRIVATE /W4 /WX /permissive- /utf-8)
    else()
      target_compile_options(gutcpp_ettin_native_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if(GUTCPP_ETTIN_MODEL_PATH)
      add_test(NAME ettin_actual_model_reranking
        COMMAND gutcpp_ettin_native_test "${GUTCPP_ETTIN_MODEL_PATH}")
      set_tests_properties(ettin_actual_model_reranking PROPERTIES TIMEOUT 240)
    endif()
  endif()
endif()
