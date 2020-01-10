#pragma once

#include "type_info.hpp"

#include "priv/mutex.hpp"

#include <vector>
#include <algorithm>

namespace trex
{

template <typename Base>
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
    };

    template <typename T>
    void register_type(const t_type_info<T>& ti)
    {
        static_assert(std::is_base_of<Base, T>::value, "Not a base class");

        priv::lock_guard l(m_register_mutex);

        type_info new_type_info;
        static_cast<trex::type_info&>(new_type_info) = ti;
        new_type_info.as_base = as_base<T>;

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

    Base* construct(const char* name, void* buf, size_t buf_size = ~size_t(0)) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;

        auto offset = info.bytes_to_align(buf);
        if (offset + info.size > buf_size) return nullptr; // buf is not big enough

        auto ptr = reinterpret_cast<uint8_t*>(buf);
        ptr += offset;

        info.default_construct_at(ptr);
        return info.as_base(ptr);
    }

    Base* alloc_and_construct(const char* name) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;

        return info.as_base(info.alloc_and_construct());
    }

private:

    template <typename T>
    static Base* as_base(void* t)
    {
        return reinterpret_cast<T*>(t);
    }

    mutable priv::mutex m_register_mutex;
    std::vector<type_info> m_registered_types;
};

}
