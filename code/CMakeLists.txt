# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_library(trex INTERFACE)
add_library(trex::trex ALIAS trex)

target_sources(trex
    INTERFACE FILE_SET HEADERS FILES
        trex/type_info.hpp
)
