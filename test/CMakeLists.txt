# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
CPMAddPackage(gh:iboB/doctest-util@0.1.4)
set_target_properties(doctest PROPERTIES FOLDER test)
set_target_properties(doctest-main PROPERTIES FOLDER test)

macro(trex_test test)
    add_doctest_lib_test(${test} trex t-${test}.cpp ${ARGN})
endmacro()

trex_test(facets)
