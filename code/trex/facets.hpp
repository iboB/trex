// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "reg.hpp"

#include <string_view>
#include <cstdint>
#include <memory>
#include <vector>
#include <cassert>
#include <stdexcept>

#include <splat/pp_util.h>

namespace trex {

using facet_id_t = uint32_t;
inline constexpr facet_id_t invalid_facet_id = facet_id_t(-1);

template <typename Registration = reg::default_registration>
class facet_domain {
    using mutex = typename Registration::mutex;
    using lock_guard = typename Registration::lock_guard;
public:
    struct facet_info {
        std::string_view name;
        explicit operator bool() const { return !name.empty(); }
    };

    uint32_t register_facet(std::string_view name) {
        if (name.empty()) {
            throw std::invalid_argument("facet name cannot be empty");
        }

        lock_guard lock(m_register_mutex);

        // search in reverse order so that finally the smallest free slot is used
        facet_id_t free_slot = facet_id_t(m_facets.size());
        for (facet_id_t i = facet_id_t(m_facets.size()); i-- > 0; ) {
            auto& f = m_facets[i];
            if (!f) {
                free_slot = i;
            }
            else if (f.name == name) {
                throw std::invalid_argument("facet name already sparse: " + std::string(name));
            }
        }
        assert(free_slot <= m_facets.size());
        if (free_slot == m_facets.size()) {
            m_facets.push_back({ name });
        }
        else {
            m_facets[free_slot] = { name };
        }
        return free_slot;
    }

    void unregister_facet(facet_id_t id) {
        lock_guard lock(m_register_mutex);
        if (id >= m_facets.size()) {
            return; // kinda safe to ignore, but maybe throw?
        }
        m_facets[id] = {};
    }

    facet_id_t get_facet_by_name(std::string_view name) const {
        lock_guard lock(m_register_mutex);
        for (facet_id_t i = 0; i < m_facets.size(); ++i) {
            if (m_facets[i].name == name) {
                return i;
            }
        }
        return invalid_facet_id;
    }

private:
    mutable mutex m_register_mutex;
    std::vector<facet_info> m_facets; // sparse
};

#define TREX_DECLARE_EXPORTED_FACET_DOMAIN(export, domain) \
    export domain& _trex_get_domain(domain* adl_tag)

#define TREX_DECLARE_FACET_DOMAIN(export, domain) \
    TREX_DECLARE_EXPORTED_FACET_DOMAIN(SPLAT_PP_EMPTY(), domain)

#define TREX_DEFINE_FACET_DOMAIN(domain) \
    domain& _trex_get_domain(domain*) { \
        static domain d; \
        return d; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    domain& _trex_get_domain(domain*)

template <typename Domain>
Domain& get_facet_domain() {
    return _trex_get_domain(static_cast<Domain*>(nullptr));
}

template <typename Domain, typename Facet>
struct facet_info_instance {
    const facet_id_t id;

    facet_info_instance(std::string_view name)
        : id(get_facet_domain<Domain>().register_facet(name))
    {}

    ~facet_info_instance() {
        get_facet_domain<Domain>().unregister_facet(id);
    }
};

#define TREX_DECLARE_EXPORTED_FACET(export, domain, facet) \
    export facet_id_t _trex_get_facet_id(domain* adl_tag, facet* adl_tag2);

#define TREX_DECLARE_FACET(export, domain, facet) \
    TREX_DECLARE_EXPORTED_FACET(SPLAT_PP_EMPTY(), domain, facet)

#define I_TREX_FACET_INSTANCE_VAR_NAME(facet) SPLAT_PP_CONCAT(_trex_facet_info_instance_, facet)

#define TREX_DEFINE_FACET(domain, facet) \
    facet_info_instance<domain, facet> I_TREX_FACET_INSTANCE_VAR_NAME(facet); \
    facet_id_t _trex_get_facet_id(domain*, facet*) { \
        return I_TREX_FACET_INSTANCE_VAR_NAME(facet).id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    facet_id_t _trex_get_facet_id(domain*, facet*)

#define TREX_DEFINE_MULTI_DOMAIN_FACET(domain, facet, name) \
    facet_id_t _trex_get_facet_id(domain*, facet*) { \
        static facet_info_instance<domain, facet> info(SPLAT_PP_STRINGIZE(facet)); \
        return info.id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    facet_id_t _trex_get_facet_id(domain*, facet*)

} // namespace trex
