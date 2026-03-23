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
  if(APPLE)
    set(VIVID_PLUGIN_SUFFIX .dylib)
  elseif(WIN32)
    set(VIVID_PLUGIN_SUFFIX .dll)
  else()
    set(VIVID_PLUGIN_SUFFIX .so)
  endif()
endif()

option(VIVID_PACKAGE_BUILD_TESTS "Build vivid-sequencers package tests" OFF)

add_library(vivid_operator_api INTERFACE)
target_include_directories(vivid_operator_api INTERFACE ${VIVID_SRC_DIR}/src)

# Minimal WebGPU discovery for runtime-linked tests
# Prefer the host runtime's libwgpu_native to avoid loading a second WebGPU
# runtime in package plugins.
set(_wgpu_include "")
set(_wgpu_lib_dir "")

if(EXISTS "${VIVID_SRC_DIR}/include/webgpu/webgpu.h")
  set(_wgpu_include "${VIVID_SRC_DIR}/include")
endif()

if(EXISTS "${VIVID_BUILD_DIR}/vivid.app/Contents/MacOS/libwgpu_native.dylib")
  set(_wgpu_lib_dir "${VIVID_BUILD_DIR}/vivid.app/Contents/MacOS")
elseif(EXISTS "${VIVID_BUILD_DIR}/libwgpu_native.dylib")
  set(_wgpu_lib_dir "${VIVID_BUILD_DIR}")
endif()

# Fallback for unusual build trees
if(_wgpu_include STREQUAL "" OR _wgpu_lib_dir STREQUAL "")
  set(_deps_dir "${VIVID_BUILD_DIR}/_deps")
  if(IS_DIRECTORY "${_deps_dir}")
    file(GLOB _dep_entries "${_deps_dir}/*")
    foreach(_entry ${_dep_entries})
      get_filename_component(_entry_name "${_entry}" NAME)
      if(_entry_name MATCHES "wgpu.*-src")
        if(_wgpu_include STREQUAL "" AND EXISTS "${_entry}/include/webgpu/webgpu.h")
          set(_wgpu_include "${_entry}/include")
        endif()
        if(_wgpu_lib_dir STREQUAL "" AND IS_DIRECTORY "${_entry}/lib")
          set(_wgpu_lib_dir "${_entry}/lib")
        endif()
      endif()
    endforeach()
  endif()
endif()

add_library(webgpu INTERFACE)
if(NOT _wgpu_include STREQUAL "")
  target_include_directories(webgpu INTERFACE "${_wgpu_include}")
endif()
if(NOT _wgpu_lib_dir STREQUAL "")
  if(EXISTS "${_wgpu_lib_dir}/libwgpu_native.dylib")
    set(WGPU_NATIVE_LIB "${_wgpu_lib_dir}/libwgpu_native.dylib")
  else()
    unset(WGPU_NATIVE_LIB CACHE)
    find_library(WGPU_NATIVE_LIB wgpu_native PATHS "${_wgpu_lib_dir}" NO_DEFAULT_PATH)
  endif()
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
  tracker
  euclidean
  pat_transform
  phase_to_midi
)

foreach(name IN LISTS VIVID_SEQUENCER_OPS)
  add_library(${name} MODULE src/${name}.cpp)
  target_link_libraries(${name} PRIVATE vivid_operator_api webgpu)
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

  set(VIVID_PACKAGE_RUNTIME_TEST_SOURCES
    ${VIVID_SRC_DIR}/src/runtime/operator_loader.cpp
    ${VIVID_SRC_DIR}/src/runtime/operator_registry.cpp
    ${VIVID_SRC_DIR}/src/runtime/port_type_registry.cpp
    ${VIVID_SRC_DIR}/src/runtime/wgsl_header_parser.cpp
  )

  add_executable(test_arpeggiator_patterns tests/test_arpeggiator_patterns.cpp)
  target_compile_features(test_arpeggiator_patterns PRIVATE cxx_std_17)
  add_test(NAME test_arpeggiator_patterns COMMAND test_arpeggiator_patterns)

  add_executable(test_state_machine tests/test_state_machine.cpp)
  target_include_directories(test_state_machine PRIVATE ${VIVID_SRC_DIR}/src)
  target_link_libraries(test_state_machine PRIVATE vivid_operator_api)
  add_test(NAME test_state_machine COMMAND test_state_machine)

  add_executable(test_pattern_algebra
    tests/test_pattern_algebra.cpp
    ${VIVID_PACKAGE_RUNTIME_TEST_SOURCES}
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

  add_executable(test_operator_smoke
    tests/test_operator_smoke.cpp
    ${VIVID_PACKAGE_RUNTIME_TEST_SOURCES}
  )
  target_include_directories(test_operator_smoke PRIVATE
    ${VIVID_SRC_DIR}/src
    ${VIVID_SRC_DIR}/deps/yyjson
    ${CMAKE_SOURCE_DIR}/tests
  )
  target_link_libraries(test_operator_smoke PRIVATE vivid_operator_api webgpu)
  if(YYJSON_LIB)
    target_link_libraries(test_operator_smoke PRIVATE ${YYJSON_LIB})
  else()
    target_link_libraries(test_operator_smoke PRIVATE yyjson)
  endif()
  add_test(NAME test_operator_smoke
    COMMAND test_operator_smoke
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_tests_properties(test_operator_smoke PROPERTIES
    ENVIRONMENT "VIVID_CORE_PLUGIN_DIR=${VIVID_BUILD_DIR}")

  add_executable(test_audio_operator_contracts
    tests/test_audio_operator_contracts.cpp
    ${VIVID_PACKAGE_RUNTIME_TEST_SOURCES}
  )
  target_include_directories(test_audio_operator_contracts PRIVATE
    ${VIVID_SRC_DIR}/src
    ${VIVID_SRC_DIR}/deps/yyjson
    ${CMAKE_SOURCE_DIR}/tests
  )
  target_link_libraries(test_audio_operator_contracts PRIVATE vivid_operator_api webgpu)
  if(YYJSON_LIB)
    target_link_libraries(test_audio_operator_contracts PRIVATE ${YYJSON_LIB})
  else()
    target_link_libraries(test_audio_operator_contracts PRIVATE yyjson)
  endif()
  add_test(NAME test_audio_operator_contracts
    COMMAND test_audio_operator_contracts
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

  add_executable(test_tracker_contract
    tests/test_tracker_contract.cpp
    ${VIVID_PACKAGE_RUNTIME_TEST_SOURCES}
  )
  target_include_directories(test_tracker_contract PRIVATE
    ${VIVID_SRC_DIR}/src
    ${VIVID_SRC_DIR}/deps/yyjson
    ${CMAKE_SOURCE_DIR}/tests
  )
  target_link_libraries(test_tracker_contract PRIVATE vivid_operator_api webgpu)
  if(YYJSON_LIB)
    target_link_libraries(test_tracker_contract PRIVATE ${YYJSON_LIB})
  else()
    target_link_libraries(test_tracker_contract PRIVATE yyjson)
  endif()
  add_test(NAME test_tracker_contract
    COMMAND test_tracker_contract
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
