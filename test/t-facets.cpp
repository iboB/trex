// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <trex/facets.hpp>
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

struct domain_a : public trex::facet_domain<trex::reg::fast_registration> {};
TREX_DECLARE_FACET_DOMAIN(domain_a);
struct domain_b : public trex::facet_domain<trex::reg::thread_safe_registration> {};
TREX_DECLARE_FACET_DOMAIN(domain_b);

struct facet_a {
    int payload;
};
TREX_DECLARE_FACET(domain_a, facet_a);

struct facet_multi {
    std::string payload;
};
TREX_DECLARE_FACET(domain_a, facet_multi);
TREX_DECLARE_FACET(domain_b, facet_multi);
TREX_DECLARE_FACET(domain_a, uint64_t);

TREX_DECLARE_FACET(domain_b, double);

TEST_CASE("domain") {
    auto& d_a = trex::get_facet_domain<domain_a>();
    auto& d_b = trex::get_facet_domain<domain_b>();

    const auto id_a = trex::get_facet_id<domain_a, facet_a>();
    CHECK(id_a != trex::invalid_facet_id);
    const auto id_multi_a = trex::get_facet_id<domain_a, facet_multi>();
    CHECK(id_multi_a != trex::invalid_facet_id);
    const auto id_multi_b = trex::get_facet_id<domain_b, facet_multi>();
    CHECK(id_multi_b != trex::invalid_facet_id);
    const auto id_uint64 = trex::get_facet_id<domain_a, uint64_t>();
    CHECK(id_uint64 != trex::invalid_facet_id);
    const auto id_double = trex::get_facet_id<domain_b, double>();
    CHECK(id_double != trex::invalid_facet_id);

    CHECK(d_a.get_facet_id_by_name("facet_a") == id_a);
    CHECK(d_a.get_facet_id_by_name("facet_multi") == id_multi_a);
    CHECK(d_b.get_facet_id_by_name("facet_multi") == id_multi_b);
    CHECK(d_a.get_facet_id_by_name("uint64_t") == id_uint64);
    CHECK(d_b.get_facet_id_by_name("double") == id_double);

    CHECK(d_a.get_facet_id_by_name("nonexistent") == trex::invalid_facet_id);
    CHECK(d_a.get_facet_id_by_name("double") == trex::invalid_facet_id);
    CHECK(d_b.get_facet_id_by_name("facet_a") == trex::invalid_facet_id);
}


/////////////////////////////////
// definitions
// keep last
TREX_DEFINE_FACET_DOMAIN(domain_a);

TREX_DEFINE_FACET(domain_a, facet_a);
TREX_DEFINE_MULTI_DOMAIN_FACET(domain_a, facet_multi);
TREX_DEFINE_FACET(domain_a, uint64_t);

TREX_DEFINE_FACET(domain_b, double);
TREX_DEFINE_MULTI_DOMAIN_FACET(domain_b, facet_multi);

TREX_DEFINE_FACET_DOMAIN(domain_b);
