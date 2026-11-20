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
if(BUILD_TESTING)
  set(GUTCPP_TEST_FILES
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
    tests/portable_test.cpp
    tests/json_test.cpp
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

# Optional executable JSON-Lines adapter for differential tests against the
# immutable compiled GutTS reference. No Node.js dependency in the library.
option(GUTCPP_DIFFERENTIAL_RUNNER "Build JSON-Lines native API differential driver" OFF)
if(GUTCPP_DIFFERENTIAL_RUNNER)
  add_executable(gutcpp_ettin_head_runner tests/ettin_head_runner.cpp)
  target_link_libraries(gutcpp_ettin_head_runner PRIVATE gutcpp::gutcpp)
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
install(EXPORT GutCPPTargets NAMESPACE gutcpp:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")
include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GutCPPConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfig.cmake"
 "${CMAKE_CURRENT_BINARY_DIR}/GutCPPConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GutCPP")
