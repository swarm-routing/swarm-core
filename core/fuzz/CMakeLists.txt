# Copyright (c) 2026 Daniel Joppi (https://github.com/swarm-routing)
# SPDX-License-Identifier: MIT OR GPL-2.0-only
#
# libFuzzer targets for the readers of untrusted bytes (#9; org rule 3), added
# by core/CMakeLists.txt only with SWARM_CORE_FUZZ=ON, with Clang:
#
#   cmake -S core -B build/fuzz -DSWARM_CORE_FUZZ=ON -DCMAKE_CXX_COMPILER=clang++
#   cmake --build build/fuzz && build/fuzz/fuzz/fuzz-wire-reader -max_total_time=60
#
# A target is fuzz-<name>.cc here plus its <name> in the list below; CI
# (core.yml, job `fuzz`) runs every fuzz-* executable it finds, so nothing else
# changes. The readers are header-only, so a target links no library.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "SWARM_CORE_FUZZ needs Clang (libFuzzer); CMAKE_CXX_COMPILER is ${CMAKE_CXX_COMPILER_ID}")
endif()

set(SWARM_CORE_FUZZ_TARGETS
  envelope
  wire-reader
)

foreach(_name IN LISTS SWARM_CORE_FUZZ_TARGETS)
  add_executable(fuzz-${_name} fuzz-${_name}.cc)
  target_compile_options(fuzz-${_name} PRIVATE -g -fsanitize=fuzzer,address,undefined
                                               -fno-sanitize-recover=all -fno-omit-frame-pointer)
  target_link_options(fuzz-${_name} PRIVATE -fsanitize=fuzzer,address,undefined)
endforeach()
