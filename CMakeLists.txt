# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(trex
    LANGUAGES CXX
)

# cpm
if(NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm" CACHE PATH "CPM source cache")
    message(STATUS "Setting cpm cache dir to: ${CPM_SOURCE_CACHE}")
endif()
include(./get_cpm.cmake)

#######################################
# cmake lib
CPMAddPackage(gh:iboB/icm@1.5.7)

list(APPEND CMAKE_MODULE_PATH
    "${icm_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(ICM_DEV_CXX_STANDARD 17)

include(icm_dev_mode)

#######################################
# options

option(TREX_BUILD_TESTS "${PROJECT_NAME}: enable testing and build tests" ${ICM_DEV_MODE})
option(TREX_BUILD_EXAMPLES "${PROJECT_NAME}: build examples" ${ICM_DEV_MODE})
option(TREX_BUILD_BENCH "${PROJECT_NAME}: build benchmarks" ${ICM_DEV_MODE})
option(TREX_BUILD_SANDBOX "${PROJECT_NAME}: build sandbox project (dev experiments)" ${ICM_DEV_MODE})
mark_as_advanced(TREX_BUILD_TESTS TREX_BUILD_EXAMPLES TREX_BUILD_BENCH TREX_BUILD_SANDBOX)

#######################################
# code
add_subdirectory(code)

#~ if(TREX_BUILD_TESTS)
#~     enable_testing()
#~     add_subdirectory(test)
#~ endif()

#~ if(TREX_BUILD_BENCH)
#~     add_subdirectory(bench)
#~ endif()

#~ if(TREX_BUILD_EXAMPLES)
#~     add_subdirectory(example)
#~ endif()

if(TREX_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()
