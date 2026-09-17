# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
CPMAddPackage(gh:iboB/splat@1.3.5)

add_library(trex INTERFACE)
add_library(trex::trex ALIAS trex)

target_sources(trex
    INTERFACE FILE_SET HEADERS FILES
        trex/type_info.hpp
)

target_link_libraries(trex INTERFACE
    splat::splat
)
