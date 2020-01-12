#pragma once

#include "type_info.hpp"

#include "priv/mutex.hpp"

#include <vector>
#include <algorithm>

namespace trex
{

template <typename Base, typename... Args>
class hierarchy
{
public:
    using base_t = Base;

    // hierarchy specific type info
    struct type_info : public trex::type_info
    {
        // we can't just reinterpret cast void* to a base pointer
        // base is not necessarily first in the parents list
        using as_base_func = Base*(*)(void* self);
        as_base_func as_base;

        using alloc_and_construct_a_f = Base* (*)(Args&&...);
        alloc_and_construct_a_f alloc_and_construct_a;

        using construct_at_a_f = Base* (*)(void*, Args&&...);
        construct_at_a_f construct_at_a;
    };

    template <typename T>
    void register_type(const t_type_info<T>& ti)
    {
        static_assert(std::is_base_of<Base, T>::value, "Not a base class");

        priv::lock_guard l(m_register_mutex);

        type_info new_type_info;
        static_cast<trex::type_info&>(new_type_info) = ti;
        new_type_info.as_base = as_base<T>;
        new_type_info.construct_at_a = construct_at_a<T>;
        new_type_info.alloc_and_construct_a = alloc_and_construct_a<T>;

        // here we don't simply add the to the list
        // to support plugins and hot reloading, we override existing types
        for (auto& t : m_registered_types)
        {
            if (t.name == ti.name)
            {
                t = std::move(new_type_info);
                return;
            }
        }

        // type was not found, so add
        m_registered_types.emplace_back(std::move(new_type_info));
    }

    type_info find_type_info(const char* name) const noexcept
    {
        priv::lock_guard lock(m_register_mutex);
        auto f = std::find_if(m_registered_types.begin(), m_registered_types.end(), [name](const type_info& t) {
            return name == t.name;
        });
        if (f == m_registered_types.end()) return {}; // no such type

        return *f;
    }

    struct buffer
    {
        buffer(void* ptr) : ptr(ptr) {}
        buffer(void* ptr, size_t size) : ptr(ptr), size(size) {}
        void* ptr;
        size_t size = ~size_t(0);
    };

    Base* construct(const char* name, buffer buf, Args&&... args) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;

        auto offset = info.bytes_to_align(buf.ptr);
        if (offset + info.size > buf.size) return nullptr; // buf is not big enough

        auto ptr = reinterpret_cast<uint8_t*>(buf.ptr);
        ptr += offset;
        return info.construct_at_a(ptr, std::forward<Args>(args)...);
    }

    Base* alloc_and_construct(const char* name, Args&&... args) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;
        return info.alloc_and_construct_a(std::forward<Args>(args)...);
    }

private:
    template <typename T>
    static Base* as_base(void* t)
    {
        return reinterpret_cast<T*>(t);
    }

    template <typename T>
    static Base* construct_at_a(void* ptr, Args&&... args)
    {
        return new (ptr) T(std::forward<Args>(args)...);
    }

    template <typename T>
    static Base* alloc_and_construct_a(Args&&... args)
    {
        return new T(std::forward<Args>(args)...);
    }

    mutable priv::mutex m_register_mutex;
    std::vector<type_info> m_registered_types;
};

}
