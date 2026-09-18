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
                throw std::invalid_argument("facet name already exists: " + std::string(name));
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

    facet_id_t get_facet_id_by_name(std::string_view name) const {
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

#define TREX_DECLARE_FACET_DOMAIN(domain) \
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
    export ::trex::facet_id_t _trex_get_facet_id(domain* adl_tag, facet* adl_tag2)

#define TREX_DECLARE_FACET(domain, facet) \
    TREX_DECLARE_EXPORTED_FACET(SPLAT_PP_EMPTY(), domain, facet)

#define I_TREX_FACET_INSTANCE_VAR_NAME(facet) SPLAT_PP_CAT(_trex_facet_info_instance_, facet)

#define TREX_DEFINE_FACET(domain, facet) \
    ::trex::facet_info_instance<domain, facet> I_TREX_FACET_INSTANCE_VAR_NAME(facet)(SPLAT_PP_STRINGIZE(facet)); \
    ::trex::facet_id_t _trex_get_facet_id(domain*, facet*) { \
        return I_TREX_FACET_INSTANCE_VAR_NAME(facet).id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    ::trex::facet_id_t _trex_get_facet_id(domain*, facet*)

#define TREX_DEFINE_MULTI_DOMAIN_FACET(domain, facet) \
    ::trex::facet_id_t _trex_get_facet_id(domain*, facet*) { \
        static ::trex::facet_info_instance<domain, facet> info(SPLAT_PP_STRINGIZE(facet)); \
        return info.id; \
    } \
    /* absolutely pointless line, which will require a semicolon at the end of the macro */ \
    ::trex::facet_id_t _trex_get_facet_id(domain*, facet*)

template <typename Domain, typename Facet>
facet_id_t get_facet_id() {
    return _trex_get_facet_id(static_cast<Domain*>(nullptr), static_cast<Facet*>(nullptr));
}

using facet_te_ptr = std::shared_ptr<void>;

namespace impl {

template <typename Facet, typename... Args>
facet_te_ptr make_facet_ptr(Args&&... args) {
    return std::make_shared<std::decay_t<Facet>>(std::forward<Args>(args)...);
}

template <typename Facet>
facet_te_ptr make_facet_ref(Facet& facet) {
    // alias nullptr
    return std::shared_ptr<std::decay_t<Facet>>(facet_te_ptr{}, &facet);
}

} // namespace impl

// dense container using a linear lookup for facet ids
// usually the best choice for the commonly expected small number of facets
struct dense_facet_container {
    std::vector<std::pair<facet_id_t, facet_te_ptr>> facets;

    facet_te_ptr& make_or_get_ptr(facet_id_t facet_id) {
        for (auto& [slot_id, ptr] : facets) {
            if (slot_id == facet_id) return ptr;
        }
        facets.push_back({facet_id, nullptr});
        return facets.back().second;
    }

    void* find(facet_id_t facet_id) const noexcept {
        for (const auto& [slot_id, ptr] : facets) {
            if (slot_id == facet_id) {
                return ptr.get();
            }
        }
        return nullptr;
    }

    void erase(facet_id_t facet_id) noexcept {
        for (auto it = facets.begin(); it != facets.end(); ++it) {
            if (it->first == facet_id) {
                facets.erase(it);
                return;
            }
        }
    }
};

// sparse container using O(1) direct indexing for facet ids
// use when you expect a larget number of facets or when facet get is performance critical
// WARNING: the size of the container will be at least as large as the largest facet id,
// so if you have a large number of registered facets this might waste a lot of memory
struct sparse_facet_container {
    std::vector<facet_te_ptr> facets;

    facet_te_ptr& make_or_get_ptr(facet_id_t facet_id) {
        if (facet_id >= facets.size()) {
            facets.resize(facet_id + 1);
        }
        return facets[facet_id];
    }

    void* find(facet_id_t facet_id) const noexcept {
        if (facet_id >= facets.size()) return nullptr;
        return facets[facet_id].get();
    }

    void erase(facet_id_t facet_id) noexcept {
        if (facet_id >= facets.size()) return;
        facets[facet_id] = nullptr;
    }
};

// container adapting an existing map type to be used as a facet container
template <template <typename...> class Map>
struct map_facet_container {
    Map<facet_id_t, facet_te_ptr> facets;

    facet_te_ptr& make_or_get_ptr(facet_id_t facet_id) {
        return facets[facet_id];
    }

    void* find(facet_id_t facet_id) const noexcept {
        auto it = facets.find(facet_id);
        if (it == facets.end()) return nullptr;
        return it->second.get();
    }

    void erase(facet_id_t facet_id) noexcept {
        facets.erase(facet_id);
    }
};

using default_facet_container = dense_facet_container;

template <typename Domain, typename Container = default_facet_container>
class facets {
    Container m_container;

    template <typename Facet>
    static facet_id_t get_facet_id() {
        return ::trex::get_facet_id<Domain, Facet>();
    }

    static facet_id_t get_facet_id(std::string_view name) {
        auto& domain = ::trex::get_facet_domain<Domain>();
        return domain.get_facet_id_by_name(name);
    }
public:
    // set default constructed
    template <typename Facet, typename... Args>
    Facet& set(Args&&... args) {
        auto id = get_facet_id<Facet>();
        auto& ptr = m_container.make_or_get_ptr(id);
        ptr = impl::make_facet_ptr<Facet>(std::forward<Args>(args)...);
        return static_cast<Facet&>(*ptr);
    }

    template <typename Facet>
    void set(Facet&& facet) {
        auto id = get_facet_id<Facet>();
        m_container.make_or_get_ptr(id) = impl::make_facet_ptr<Facet>(std::forward<Facet>(facet));
    }

    template <typename Facet>
    void set_ref(Facet& f) {
        auto id = get_facet_id<Facet>();
        m_container.make_or_get_ptr(id) = impl::make_facet_ref(f);
    }

    template <typename Facet>
    void set_shared(std::shared_ptr<Facet> f) {
        auto id = get_facet_id<Facet>();

        if (!f) {
            m_container.erase(id);
        }
        else {
            m_container.make_or_get_ptr(id) = std::move(f);
        }
    }

    // make sure you know what you're doing, this is not type-safe
    void set_unsafe(std::string_view name, facet_te_ptr ptr) {
        auto id = get_facet_id(name);
        if (id == invalid_facet_id) {
            throw std::invalid_argument("facet name not registered: " + std::string(name));
        }
        m_container.make_or_get_ptr(id) = std::move(ptr);
    }

    template <typename Facet>
    void reset() {
        auto id = get_facet_id<Facet>();
        m_container.erase(id);
    }

    void reset(std::string_view name) {
        auto id = get_facet_id(name);
        m_container.erase(id);
    }

    template <typename Facet>
    bool has() const noexcept {
        auto id = get_facet_id<Facet>();
        return m_container.find(id) != nullptr;
    }

    bool has(std::string_view name) const noexcept {
        auto id = get_facet_id(name);
        return m_container.find(id) != nullptr;
    }

    // get if it exists, otherwise create a default constructed one
    template <typename Facet>
    Facet& get_default() {
        auto id = get_facet_id<Facet>();
        auto& ptr = m_container.make_or_get_ptr(id);
        if (!ptr) {
            ptr = impl::make_facet_ptr<Facet>();
        }
        return *static_cast<Facet*>(ptr.get());
    }

    template <typename Facet>
    auto& get_default_pl() {
        auto& facet = get_default<Facet>();
        return facet.payload;
    }

    template <typename Facet>
    Facet* get() const noexcept {
        auto id = get_facet_id<Facet>();
        return static_cast<Facet*>(m_container.find(id));
    }

    template <typename Facet>
    auto* get_pl() const noexcept {
        auto* facet = get<Facet>();
        return facet ? &facet->payload : nullptr;
    }

    void* get(std::string_view name) const noexcept {
        auto id = get_facet_id(name);
        return m_container.find(id);
    }
};

} // namespace trex
