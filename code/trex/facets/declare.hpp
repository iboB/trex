// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <splat/pp_util.h>

#define TREX_DECLARE_EXPORTED_FACET_DOMAIN(export, domain) \
    export domain& _trex_get_facet_domain(domain* adl_tag)

#define TREX_DECLARE_FACET_DOMAIN(domain) \
    TREX_DECLARE_EXPORTED_FACET_DOMAIN(SPLAT_PP_EMPTY(), domain)

#define TREX_DECLARE_EXPORTED_FACET(export, domain, facet) \
    export ::trex::facet_id _trex_get_facet_id(domain* adl_tag, facet* adl_tag2)

#define TREX_DECLARE_FACET(domain, facet) \
    TREX_DECLARE_EXPORTED_FACET(SPLAT_PP_EMPTY(), domain, facet)
