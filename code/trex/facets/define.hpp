// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "get.hpp"
#include <string_view>
#include <splat/pp_util.h>

namespace trex {
template <typename Domain, typename Facet>
struct facet_info_instance {
    const facet_id id;

    facet_info_instance(std::string_view name)
        : id(get_facet_domain<Domain>().register_facet(name))
    {}

    ~facet_info_instance() {
        get_facet_domain<Domain>().unregister_facet(id);
    }
};
}

#define TREX_DEFINE_FACET_DOMAIN(domain) \
    domain& _trex_get_facet_domain(domain*) { \
        static domain d; \
        return d; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    domain& _trex_get_facet_domain(domain*)

#define I_TREX_FACET_INSTANCE_VAR_NAME(facet) SPLAT_PP_CAT(_trex_facet_info_instance_, facet)

#define TREX_DEFINE_FACET(domain, facet) \
    ::trex::facet_info_instance<domain, facet> I_TREX_FACET_INSTANCE_VAR_NAME(facet)(SPLAT_PP_STRINGIZE(facet)); \
    ::trex::facet_id _trex_get_facet_id(domain*, facet*) { \
        return I_TREX_FACET_INSTANCE_VAR_NAME(facet).id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    ::trex::facet_id _trex_get_facet_id(domain*, facet*)

#define TREX_DEFINE_MULTI_DOMAIN_FACET(domain, facet) \
    ::trex::facet_id _trex_get_facet_id(domain*, facet*) { \
        static ::trex::facet_info_instance<domain, facet> info(SPLAT_PP_STRINGIZE(facet)); \
        return info.id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    ::trex::facet_id _trex_get_facet_id(domain*, facet*)

