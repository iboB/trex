// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "facet_id.hpp"
#include "get.hpp"

#include <vector>
#include <memory>
#include <utility>
#include <cassert>
#include <stdexcept>
#include <string_view>

namespace trex {

using facet_te_ptr = std::shared_ptr<void>;

namespace impl {

template <typename Facet>
std::shared_ptr<Facet> make_facet_ref(Facet& facet) {
    // alias nullptr
    return std::shared_ptr<Facet>(facet_te_ptr{}, &facet);
}

} // namespace impl

// dense container using a linear lookup for facet ids
// usually the best choice for the commonly expected small number of facets
struct dense_facet_container {
    std::vector<std::pair<facet_id, facet_te_ptr>> facets;

    facet_te_ptr& make_or_get_ptr(facet_id fid) {
        for (auto& [slot_id, ptr] : facets) {
            if (slot_id == fid) return ptr;
        }
        facets.push_back({fid, nullptr});
        return facets.back().second;
    }

    void* find(facet_id fid) const noexcept {
        for (const auto& [slot_id, ptr] : facets) {
            if (slot_id == fid) {
                return ptr.get();
            }
        }
        return nullptr;
    }

    facet_te_ptr erase(facet_id fid) {
        for (auto it = facets.begin(); it != facets.end(); ++it) {
            if (it->first == fid) {
                auto ret = std::move(it->second);
                facets.erase(it);
                return ret;
            }
        }
        return {};
    }
};

// sparse container using O(1) direct indexing for facet ids
// use when you expect a larget number of facets or when facet get is performance critical
// WARNING: the size of the container will be at least as large as the largest facet id,
// so if you have a large number of registered facets this might waste a lot of memory
struct sparse_facet_container {
    std::vector<facet_te_ptr> facets;

    facet_te_ptr& make_or_get_ptr(facet_id fid) {
        if (fid >= facets.size()) {
            facets.resize(fid + 1);
        }
        return facets[fid];
    }

    void* find(facet_id fid) const noexcept {
        if (fid >= facets.size()) return nullptr;
        return facets[fid].get();
    }

    facet_te_ptr erase(facet_id fid) noexcept {
        if (fid >= facets.size()) return {};
        return std::exchange(facets[fid], nullptr);
    }
};

// container adapting an existing map type to be used as a facet container
template <template <typename...> class Map>
struct map_facet_container {
    Map<facet_id, facet_te_ptr> facets;

    facet_te_ptr& make_or_get_ptr(facet_id fid) {
        return facets[fid];
    }

    void* find(facet_id fid) const noexcept {
        auto it = facets.find(fid);
        if (it == facets.end()) return nullptr;
        return it->second.get();
    }

    facet_te_ptr erase(facet_id fid) {
        auto it = facets.find(fid);
        if (it == facets.end()) return {};
        auto ret = std::move(it->second);
        facets.erase(it);
        return ret;
    }
};

using default_facet_container = dense_facet_container;

template <typename Container>
class facet_set_guard {
    Container& m_container;
    facet_id m_facet_id;
    facet_te_ptr m_old_facet_ptr;
public:
    facet_set_guard(Container& container, facet_id fid, facet_te_ptr newptr)
        : m_container(container)
        , m_facet_id(fid)
    {
        auto& cur = m_container.make_or_get_ptr(fid);
        m_old_facet_ptr = cur;
        cur = std::move(newptr);
    }

    ~facet_set_guard() {
        if (m_old_facet_ptr) {
            m_container.make_or_get_ptr(m_facet_id) = std::move(m_old_facet_ptr);
        }
        else {
            m_container.erase(m_facet_id);
        }
    }
};

template <typename T>
class facet_payload_guard {
    T& m_payload;
    T m_old_value;
public:
    template <typename U>
    facet_payload_guard(T& payload, U&& newValue)
        : m_payload(payload)
        , m_old_value(std::exchange(payload, std::forward<U>(newValue)))
    {}

    ~facet_payload_guard() {
        m_payload = std::move(m_old_value);
    }

    T& operator*() { return m_payload; }
    T* operator->() { return &m_payload; }
};

template <typename Domain, typename Container = default_facet_container>
class facets {
    Container m_container;

    template <typename Facet>
    static facet_id get_facet_id() {
        return ::trex::get_facet_id<Domain, Facet>();
    }

    static facet_id get_facet_id(std::string_view name) {
        auto& domain = ::trex::get_facet_domain<Domain>();
        return domain.get_facet_id_by_name(name);
    }
public:
    void set_unsafe(facet_id id, facet_te_ptr ptr) {
        if (!ptr) {
            m_container.erase(id);
        }
        else {
            m_container.make_or_get_ptr(id) = std::move(ptr);
        }
    }

    template <typename Facet>
    void set_shared(std::shared_ptr<Facet> f) {
        auto id = get_facet_id<Facet>();
        set_unsafe(id, std::move(f));
    }

    // set default constructed
    template <typename Facet, typename... Args>
    Facet& set(Args&&... args) {
        auto ptr = std::make_shared<Facet>(std::forward<Args>(args)...);
        auto& ret = *ptr;
        set_shared(std::move(ptr));
        return ret;
    }

    template <typename Facet>
    void set(Facet&& facet) {
        set_shared(std::make_shared<std::decay_t<Facet>>(std::forward<Facet>(facet)));
    }

    template <typename Facet>
    void set_ref(Facet& f) {
        set_shared(impl::make_facet_ref(f));
    }

    // make sure you know what you're doing, this is not type-safe
    void set_unsafe(std::string_view name, facet_te_ptr ptr) {
        auto id = get_facet_id(name);
        if (id == invalid_facet_id) {
            throw std::invalid_argument("facet name not registered: " + std::string(name));
        }
        set_unsafe(id, std::move(ptr));
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
            ptr = std::make_shared<Facet>();
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

    using scoped_set_guard_t = facet_set_guard<Container>;

    template <typename Facet>
    scoped_set_guard_t scoped_set_shared(std::shared_ptr<Facet> newptr) {
        auto id = get_facet_id<Facet>();
        return {m_container, id, std::move(newptr)};
    }

    template <typename Facet>
    scoped_set_guard_t scoped_set(Facet&& f) {
        return scoped_set_shared(std::make_shared<std::decay_t<Facet>>(std::forward<Facet>(f)));
    }

    template <typename Facet>
    scoped_set_guard_t scoped_set_ref(Facet& f) {
        return scoped_set_shared(impl::make_facet_ref(f));
    }

    template <typename Facet>
    scoped_set_guard_t scoped_reset() {
        auto id = get_facet_id<Facet>();
        return {m_container, id, {}};
    }

    template <typename Facet, typename T>
    facet_payload_guard<decltype(Facet::payload)> scoped_pl_set(T&& t) {
        auto* pl = get_pl<Facet>();
        assert(pl);
        return {*pl, std::forward<T>(t)};
    }
};

} // namespace trex