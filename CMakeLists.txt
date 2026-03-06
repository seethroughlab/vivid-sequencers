cmake_minimum_required(VERSION 3.16)
project(vivid_sequencers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED VIVID_SRC_DIR)
  set(VIVID_SRC_DIR "${CMAKE_SOURCE_DIR}/../vivid")
endif()
if(NOT DEFINED VIVID_BUILD_DIR)
  set(VIVID_BUILD_DIR "${VIVID_SRC_DIR}/build")
endif()
if(NOT DEFINED VIVID_PLUGIN_SUFFIX)
  set(VIVID_PLUGIN_SUFFIX .dylib)
endif()

option(VIVID_PACKAGE_BUILD_TESTS "Build vivid-sequencers package tests" OFF)

add_library(vivid_operator_api INTERFACE)
target_include_directories(vivid_operator_api INTERFACE ${VIVID_SRC_DIR}/src)

# Minimal WebGPU discovery for runtime-linked tests
set(_wgpu_include "")
set(_wgpu_lib_dir "")
set(_deps_dir "${VIVID_BUILD_DIR}/_deps")
if(IS_DIRECTORY "${_deps_dir}")
  file(GLOB _dep_entries "${_deps_dir}/*")
  foreach(_entry ${_dep_entries})
    get_filename_component(_entry_name "${_entry}" NAME)
    if(_entry_name MATCHES "wgpu.*-src")
      if(EXISTS "${_entry}/include/webgpu/webgpu.h")
        set(_wgpu_include "${_entry}/include")
        if(IS_DIRECTORY "${_entry}/lib")
          set(_wgpu_lib_dir "${_entry}/lib")
        endif()
      endif()
    endif()
  endforeach()
endif()
if(_wgpu_include STREQUAL "")
  if(EXISTS "${VIVID_SRC_DIR}/include/webgpu/webgpu.h")
    set(_wgpu_include "${VIVID_SRC_DIR}/include")
  endif()
endif()
if(_wgpu_lib_dir STREQUAL "")
  if(EXISTS "${VIVID_BUILD_DIR}/libwgpu_native.dylib")
    set(_wgpu_lib_dir "${VIVID_BUILD_DIR}")
  endif()
endif()

add_library(webgpu INTERFACE)
if(NOT _wgpu_include STREQUAL "")
  target_include_directories(webgpu INTERFACE "${_wgpu_include}")
endif()
if(NOT _wgpu_lib_dir STREQUAL "")
  find_library(WGPU_NATIVE_LIB wgpu_native PATHS "${_wgpu_lib_dir}" NO_DEFAULT_PATH)
  if(WGPU_NATIVE_LIB)
    target_link_libraries(webgpu INTERFACE "${WGPU_NATIVE_LIB}")
  endif()
endif()

set(VIVID_SEQUENCER_OPS
  sequencer
  drum_sequencer
  pattern_seq
  note_pattern
  note_duration
  arpeggiator
  chord_progression
  state_machine
)

foreach(name IN LISTS VIVID_SEQUENCER_OPS)
  add_library(${name} MODULE src/${name}.cpp)
  target_link_libraries(${name} PRIVATE vivid_operator_api)
  set_target_properties(${name} PROPERTIES
    PREFIX ""
    SUFFIX "${VIVID_PLUGIN_SUFFIX}"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()

add_custom_target(operators DEPENDS ${VIVID_SEQUENCER_OPS})

add_executable(test_package_manifest tests/test_package_manifest.cpp)
target_compile_features(test_package_manifest PRIVATE cxx_std_17)

if(VIVID_PACKAGE_BUILD_TESTS)
  enable_testing()

  add_executable(test_arpeggiator_patterns tests/test_arpeggiator_patterns.cpp)
  target_compile_features(test_arpeggiator_patterns PRIVATE cxx_std_17)
  add_test(NAME test_arpeggiator_patterns COMMAND test_arpeggiator_patterns)

  add_executable(test_state_machine tests/test_state_machine.cpp)
  target_include_directories(test_state_machine PRIVATE ${VIVID_SRC_DIR}/src)
  target_link_libraries(test_state_machine PRIVATE vivid_operator_api)
  add_test(NAME test_state_machine COMMAND test_state_machine)

  add_executable(test_pattern_algebra
    tests/test_pattern_algebra.cpp
    ${VIVID_SRC_DIR}/src/runtime/operator_loader.cpp
    ${VIVID_SRC_DIR}/src/runtime/operator_registry.cpp
    ${VIVID_SRC_DIR}/src/runtime/wgsl_header_parser.cpp
    ${VIVID_SRC_DIR}/src/runtime/graph.cpp
    ${VIVID_SRC_DIR}/src/runtime/scheduler.cpp
  )
  target_include_directories(test_pattern_algebra PRIVATE
    ${VIVID_SRC_DIR}/src
    ${VIVID_SRC_DIR}/deps/yyjson
  )
  target_link_libraries(test_pattern_algebra PRIVATE vivid_operator_api webgpu)

  find_library(YYJSON_LIB yyjson PATHS "${VIVID_BUILD_DIR}" NO_DEFAULT_PATH)
  if(YYJSON_LIB)
    target_link_libraries(test_pattern_algebra PRIVATE ${YYJSON_LIB})
  else()
    add_library(yyjson STATIC ${VIVID_SRC_DIR}/deps/yyjson/yyjson.c)
    target_include_directories(yyjson PUBLIC ${VIVID_SRC_DIR}/deps/yyjson)
    target_link_libraries(test_pattern_algebra PRIVATE yyjson)
  endif()

  add_test(NAME test_pattern_algebra
    COMMAND test_pattern_algebra
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(test_pattern_algebra PROPERTIES
    ENVIRONMENT "VIVID_CORE_PLUGIN_DIR=${VIVID_BUILD_DIR}")
endif()
