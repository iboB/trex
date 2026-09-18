// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "facet_id.hpp"
#include "../reg.hpp"

#include <string_view>
#include <stdexcept>
#include <cassert>
#include <vector>

namespace trex {

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
        facet_id free_slot = facet_id(m_facets.size());
        for (facet_id i = facet_id(m_facets.size()); i-- > 0; ) {
            auto& f = m_facets[i];
            if (!f) {
                free_slot = i;
            }
            else if (f.name == name) {
                throw std::invalid_argument("facet name already exists: " + std::string(name));
            }
        }
        assert(free_slot <= m_facets.size());
        if (free_slot == m_facets.size()) {
            m_facets.push_back({name});
        }
        else {
            m_facets[free_slot] = {name};
        }
        return free_slot;
    }

    void unregister_facet(facet_id id) {
        lock_guard lock(m_register_mutex);
        if (id >= m_facets.size()) {
            return; // kinda safe to ignore, but maybe throw?
        }
        m_facets[id] = {};
    }

    facet_id get_facet_id_by_name(std::string_view name) const {
        lock_guard lock(m_register_mutex);
        for (facet_id i = 0; i < m_facets.size(); ++i) {
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

} // namespace trex
