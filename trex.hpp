#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <algorithm>

namespace trex
{

namespace priv
{
// returns how many bytes from the pointer on one needs to move
// in order to properly construct a type with a given alignment
inline size_t bytes_to_align(const void* ptr, size_t alignment)
{
    auto nptr = reinterpret_cast<uintptr_t>(ptr);
    auto naddr = ((nptr + alignment - 1) / alignment) // divide rounding up
        * alignment; // and scale
    return naddr - nptr;
}
}

using default_constructor_func = void(*)(void* ptr);
// using destructor_func = void(*)(void* ptr);

struct type_info
{
    const char* name = nullptr;
    size_t size;
    size_t alignment;

    default_constructor_func default_constructor;
    // destructor_func destructor;
};

template <typename Base>
class hierarchy
{
public:
    Base* construct(const char* name, void* buf, size_t buf_size = ~size_t(0)) const
    {
        auto f = std::find_if(m_registered_types.begin(), m_registered_types.end(), [name](const type_info& t) {
            return strcmp(name, t.name) == 0;
        });
        if (f == m_registered_types.end()) return nullptr; // no such type

        auto& info = *f;

        auto offset = priv::bytes_to_align(buf, info.alignment);
        if (offset + info.size > buf_size) return nullptr; // buf is not enough

        auto ptr = reinterpret_cast<uint8_t*>(buf);
        ptr += offset;
        info.default_constructor(ptr);

        return info.as_base(ptr);
    }

    // hierarchy specific type info
    struct type_info : public trex::type_info
    {
        // we can't just reinterpret cast void* to a base pointer
        // base is not necessarily first in the parents list
        using as_base_func = Base*(*)(void* self);
        as_base_func as_base;
    };
private:
    std::vector<type_info> m_registered_types;
};

}
