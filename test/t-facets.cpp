// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#include <trex/facets.hpp>
#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>

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

template <typename Domain>
void test_domain_manual() {
    auto& dom = trex::get_facet_domain<Domain>();
    auto foo_id = dom.register_facet("foo");
    CHECK(foo_id != trex::invalid_facet_id);

    CHECK_THROWS_WITH(dom.register_facet("foo"), "facet name already exists: foo");
    dom.unregister_facet(foo_id);
    dom.unregister_facet(trex::invalid_facet_id); // must be safe
}

TEST_CASE("domain manual") {
    test_domain_manual<domain_a>();
    test_domain_manual<domain_b>();
}

template <typename Container>
void test_facets() {
    trex::facets<domain_a, Container> fa;
    CHECK(fa.template pget<facet_a>() == nullptr);
    CHECK(fa.template pget_pl<facet_a>() == nullptr);
    CHECK(fa.pget("facet_a") == nullptr);
    CHECK(fa.pget("foo") == nullptr);

    fa.template get_pl<facet_a>() = 42;
    CHECK(fa.template has<facet_a>());
    {
        auto f = fa.template pget<facet_a>();
        REQUIRE(f);
        CHECK(f->payload == 42);
        CHECK(fa.template pget<facet_a>() == f);
        CHECK(&fa.template get<facet_a>() == f.get());
        CHECK(fa.template pget_pl<facet_a>().get() == &f->payload);
        CHECK(fa.pget("facet_a") == f);
    }
    fa.template reset<facet_a>();
    CHECK_FALSE(fa.template has<facet_a>());

    fa.reset(facet_multi{"hello"});
    CHECK(fa.template has<facet_multi>());
    {
        auto f = fa.template pget<facet_multi>();
        REQUIRE(f);
        CHECK(f->payload == "hello");
        CHECK(fa.pget("facet_multi") == f);
    }

    facet_multi ref_share = {"ref"};
    fa.reset_ref(ref_share);
    CHECK(fa.template pget<facet_multi>().get() == &ref_share);
    CHECK(fa.template get_pl<facet_multi>() == "ref");

    auto shared_uint = std::make_shared<uint64_t>(123);
    fa.reset_shared(shared_uint);
    CHECK(shared_uint.use_count() == 2);
    CHECK(fa.template has<uint64_t>());
    CHECK(fa.pget("uint64_t") == shared_uint);
    CHECK(fa.template get<uint64_t>() == 123);
    fa.reset_name("uint64_t");
    CHECK_FALSE(fa.template has<uint64_t>());

    fa.reset(uint64_t(53));
    CHECK(fa.template has<uint64_t>());
    CHECK(fa.template get<uint64_t>() == 53);

    fa.reset_name("uint64_t", shared_uint);
    CHECK(fa.template get<uint64_t>() == 123);

    {
        auto ss = fa.scoped_reset(uint64_t(42));
        CHECK(fa.template get<uint64_t>() == 42);
    }
    CHECK(fa.template pget<uint64_t>() == shared_uint);

    {
        auto ss = fa.template scoped_reset<uint64_t>();
        CHECK_FALSE(fa.template has<uint64_t>());
    }
    CHECK(fa.template pget<uint64_t>() == shared_uint);

    {
        auto ss = fa.template scoped_pl_set<facet_multi>("scoped");
        CHECK(ref_share.payload == "scoped");
        *ss = "foo";
        CHECK(ref_share.payload == "foo");
        CHECK(ss->length() == 3);
    }
    CHECK(ref_share.payload == "ref");

    trex::facets<domain_b, Container> fb;
    fb.reset_ref(ref_share);
    CHECK(fb.template pget<facet_multi>().get() == &ref_share);
    CHECK(fb.template get_pl<facet_multi>() == "ref");

    auto pd = std::make_shared<double>(42);

    CHECK(fb.get_or_init([] { return 3.5; }) == 3.5);
    CHECK(fb.get_or_init([] { return 4.; }) == 3.5);
    CHECK(fb.get_or_init([&] { return pd; }) == 3.5);
    fb.template reset<double>();
    CHECK_FALSE(fb.template has<double>());
    CHECK(fb.get_or_init([&] { return pd; }) == 42);
    CHECK(fb.template pget<double>() == pd);
    CHECK(fb.get_or_init([] { return 4.; }) == 42);
}

TEST_CASE("facets") {
    test_facets<trex::dense_facet_container>();
    test_facets<trex::sparse_facet_container>();
    test_facets<trex::map_facet_container<std::map>>();
    test_facets<trex::map_facet_container<std::unordered_map>>();
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
