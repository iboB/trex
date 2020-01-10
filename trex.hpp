#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <cassert>

namespace trex
{

namespace priv
{

#if TREX_THREAD_SAFE_REGISTRATION
using mutex = std::mutex;
using lock_guard = std::lock_guard;
#else
struct mutex
{
    void lock() {}
    void unlock() {}
};
struct lock_guard
{
    lock_guard(mutex&) {};
};
#endif

inline bool eq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

template <typename T>
void default_constructor(void* ptr)
{
    new (ptr) T;
}

template <typename T>
void* alloc_and_construct()
{
    return new T;
}

template <typename Base, typename T>
Base* as_base(void* t)
{
    return reinterpret_cast<T*>(t);
}

// returns how many bytes from the pointer on one needs to move
// in order to properly construct a type with a given alignment
inline size_t bytes_to_align(const void* ptr, size_t alignment)
{
    auto nptr = reinterpret_cast<uintptr_t>(ptr);
    auto naddr = ((nptr + alignment - 1) / alignment) // divide rounding up
        * alignment; // and scale
    return naddr - nptr;
}

} // namespace priv

using default_constructor_func = void(*)(void* ptr);
using alloc_and_construct_func = void*(*)();
// using destructor_func = void(*)(void* ptr);

struct type_info
{
    explicit operator bool() const { return !!name; }

    const char* name = nullptr;
    size_t size;
    size_t alignment;

    default_constructor_func default_constructor;
    alloc_and_construct_func alloc_and_construct;
    // destructor_func destructor;
};

template <typename T>
type_info make_type_info(const char* name)
{
    type_info ret;
    ret.name = name;
    ret.size = sizeof(T);
    ret.alignment = std::alignment_of<T>::value;
    ret.default_constructor = priv::default_constructor<T>;
    ret.alloc_and_construct = priv::alloc_and_construct<T>;
    return ret;
}

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
    void register_type(const trex::type_info& ti)
    {
        static_assert(std::is_base_of<Base, T>::value, "Not a base class");

        priv::lock_guard l(m_register_mutex);

        type_info new_type_info;
        static_cast<trex::type_info&>(new_type_info) = ti;
        new_type_info.as_base = priv::as_base<Base, T>;

        // here we don't simply add the to the list
        // to support plugins and hot reloading, we override existing types
        for (auto& t : m_registered_types)
        {
            if (priv::eq(t.name, ti.name))
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
            return priv::eq(name, t.name);
        });
        if (f == m_registered_types.end()) return {}; // no such type

        return *f;
    }

    Base* construct(const char* name, void* buf, size_t buf_size = ~size_t(0)) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;

        auto offset = priv::bytes_to_align(buf, info.alignment);
        if (offset + info.size > buf_size) return nullptr; // buf is not big enough

        auto ptr = reinterpret_cast<uint8_t*>(buf);
        ptr += offset;

        info.default_constructor(ptr);
        return info.as_base(ptr);
    }

    Base* alloc_and_construct(const char* name) const
    {
        auto info = find_type_info(name);
        if (!info) return nullptr;

        return info.as_base(info.alloc_and_construct());
    }

private:

    mutable priv::mutex m_register_mutex;
    std::vector<type_info> m_registered_types;
};

} // namespace trex
