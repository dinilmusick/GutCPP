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
    endif()
  endforeach()
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
