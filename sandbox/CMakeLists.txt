# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_executable(trex-sandbox
    sandbox.cpp
)
target_link_libraries(trex-sandbox PRIVATE trex::trex)
set_target_properties(trex-sandbox PROPERTIES FOLDER sandbox)
