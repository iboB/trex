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

    facet_te_ptr find(facet_id fid) const noexcept {
        for (const auto& [slot_id, ptr] : facets) {
            if (slot_id == fid) {
                return ptr;
            }
        }
        return {};
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

    facet_te_ptr find(facet_id fid) const noexcept {
        if (fid >= facets.size()) return {};
        return facets[fid];
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

    facet_te_ptr find(facet_id fid) const noexcept {
        auto it = facets.find(fid);
        if (it == facets.end()) return {};
        return it->second;
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
class facet_reset_guard {
    Container& m_container;
    facet_id m_facet_id;
    facet_te_ptr m_old_facet_ptr;
public:
    facet_reset_guard(Container& container, facet_id fid, facet_te_ptr newptr)
        : m_container(container)
        , m_facet_id(fid)
    {
        auto& cur = m_container.make_or_get_ptr(fid);
        m_old_facet_ptr = cur;
        cur = std::move(newptr);
    }

    ~facet_reset_guard() {
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
    std::shared_ptr<T> m_payload;
    T m_old_value;
public:
    template <typename U>
    facet_payload_guard(std::shared_ptr<T> payload, U&& newValue)
        : m_payload(std::move(payload))
        , m_old_value(std::exchange(*m_payload, std::forward<U>(newValue)))
    {}

    ~facet_payload_guard() {
        *m_payload = std::move(m_old_value);
    }

    T& operator*() { return *m_payload; }
    T* operator->() { return m_payload.get(); }
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
    template <typename Facet>
    bool has() const noexcept {
        auto id = get_facet_id<Facet>();
        return !!m_container.find(id);
    }

    bool has_name(std::string_view name) const noexcept {
        auto id = get_facet_id(name);
        return !!m_container.find(id);
    }

    // make sure you know what you're doing, this is not type-safe
    void reset_id(facet_id id, facet_te_ptr ptr) {
        if (!ptr) {
            m_container.erase(id);
        }
        else {
            m_container.make_or_get_ptr(id) = std::move(ptr);
        }
    }

    template <typename Facet>
    void reset_shared(std::shared_ptr<Facet> f) {
        auto id = get_facet_id<Facet>();
        reset_id(id, std::move(f));
    }

    template <typename Facet>
    void reset(Facet&& facet) {
        reset_shared(std::make_shared<std::decay_t<Facet>>(std::forward<Facet>(facet)));
    }

    template <typename Facet>
    void reset_ref(Facet& f) {
        reset_shared(impl::make_facet_ref(f));
    }

    // make sure you know what you're doing, this is not type-safe
    void reset_name(std::string_view name, facet_te_ptr ptr) {
        auto id = get_facet_id(name);
        if (id == invalid_facet_id) {
            throw std::invalid_argument("facet name not registered: " + std::string(name));
        }
        reset_id(id, std::move(ptr));
    }

    template <typename Facet>
    void reset() {
        auto id = get_facet_id<Facet>();
        m_container.erase(id);
    }

    void reset_name(std::string_view name) {
        auto id = get_facet_id(name);
        m_container.erase(id);
    }

    template <typename InitFunc>
    void* get_or_init_id(facet_id id, InitFunc&& init) {
        auto& ptr = m_container.make_or_get_ptr(id);
        if (!ptr) {
            ptr = std::forward<InitFunc>(init)();
        }
        return ptr.get();
    }

    template <typename InitFunc>
    auto& get_or_init(InitFunc&& init) {
        using func_return_type = decltype(init());

        if constexpr (std::is_convertible_v<func_return_type, facet_te_ptr>) {
            // func returns shared_ptr<Facet>
            using Facet = std::decay_t<decltype(*init())>;
            auto id = get_facet_id<Facet>();
            auto p = get_or_init_id(id, std::forward<InitFunc>(init));
            return *static_cast<Facet*>(p);
        }
        else {
            // func returns asset value
            using Facet = std::decay_t<func_return_type>;
            auto id = get_facet_id<Facet>();
            auto p = get_or_init_id(id, [init = std::move(init)]() {
                return std::make_shared<Facet>(std::move(init)());
            });
            return *static_cast<Facet*>(p);
        }
    }

    // get if it exists, otherwise create a default constructed one
    template <typename Facet>
    Facet& get() {
        auto id = get_facet_id<Facet>();
        auto p = get_or_init_id(id, []() {
            return std::make_shared<Facet>();
        });
        return *static_cast<Facet*>(p);
    }

    template <typename Facet>
    auto& get_pl() {
        auto& facet = get<Facet>();
        return facet.payload;
    }

    template <typename Facet>
    std::shared_ptr<Facet> pget() const noexcept {
        auto id = get_facet_id<Facet>();
        return std::static_pointer_cast<Facet>(m_container.find(id));
    }

    template <typename Facet>
    std::shared_ptr<decltype(Facet::payload)> pget_pl() const noexcept {
        auto ptr = pget<Facet>();
        if (!ptr) return {};
        return std::shared_ptr<decltype(Facet::payload)>(ptr, &ptr->payload);
    }

    facet_te_ptr pget(std::string_view name) const noexcept {
        auto id = get_facet_id(name);
        return m_container.find(id);
    }

    using scoped_reset_guard_t = facet_reset_guard<Container>;

    template <typename Facet>
    scoped_reset_guard_t scoped_reset_shared(std::shared_ptr<Facet> newptr) {
        auto id = get_facet_id<Facet>();
        return {m_container, id, std::move(newptr)};
    }

    template <typename Facet>
    scoped_reset_guard_t scoped_reset(Facet&& f) {
        return scoped_reset_shared(std::make_shared<std::decay_t<Facet>>(std::forward<Facet>(f)));
    }

    template <typename Facet>
    scoped_reset_guard_t scoped_reset_ref(Facet& f) {
        return scoped_reset_shared(impl::make_facet_ref(f));
    }

    template <typename Facet>
    scoped_reset_guard_t scoped_reset() {
        auto id = get_facet_id<Facet>();
        return {m_container, id, {}};
    }

    template <typename Facet, typename T>
    facet_payload_guard<decltype(Facet::payload)> scoped_pl_set(T&& t) {
        auto pl = pget_pl<Facet>();
        assert(pl);
        return {pl, std::forward<T>(t)};
    }
};

} // namespace trex