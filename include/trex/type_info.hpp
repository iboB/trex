// T-Rex
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace trex
{

struct type_info
{
    using v_pv_f = void(*)(void*);
    using pv_v_f = void*(*)();

    std::string_view name;
    size_t size = 0;
    size_t alignment = 0;
    v_pv_f default_construct_at = nullptr; // call default constructor on an existing pointer
    pv_v_f alloc_and_construct = nullptr; // allocate and construct an object of that type

    explicit operator bool() { return !name.empty(); }

    // returns how many bytes from the pointer on one needs to move
    // in order to properly construct a type with a given alignment
    size_t bytes_to_align(const void* ptr)
    {
        auto nptr = reinterpret_cast<uintptr_t>(ptr);
        auto naddr = ((nptr + alignment - 1) / alignment) // divide rounding up
            * alignment; // and scale
        return naddr - nptr;
    }
};

namespace priv
{

template <typename T> void default_construct_at(void* ptr) { new (ptr) T; }
template <typename T> typename std::enable_if<std::is_default_constructible<T>::value, type_info::v_pv_f>::type
get_default_construct_at_func() { return default_construct_at<T>; }
template <typename T> typename std::enable_if<! std::is_default_constructible<T>::value, type_info::v_pv_f>::type
get_default_construct_at_func() { return nullptr; }

template <typename T> void* alloc_and_construct() { return new T; }
template <typename T> typename std::enable_if<std::is_default_constructible<T>::value, type_info::pv_v_f>::type
get_alloc_and_construct_func() { return alloc_and_construct<T>; }
template <typename T> typename std::enable_if<! std::is_default_constructible<T>::value, type_info::pv_v_f>::type
get_alloc_and_construct_func() { return nullptr; }

} // namespace priv

template <typename T>
void set_missing_traits_to_info(type_info& info)
{
    if (!info.size) info.size = sizeof(T);
    if (!info.alignment) info.alignment = std::alignment_of<T>::value;
    if (!info.default_construct_at) info.default_construct_at = priv::get_default_construct_at_func<T>();
    if (!info.alloc_and_construct) info.alloc_and_construct = priv::get_alloc_and_construct_func<T>();
}

// used to cary the type name with the info
template <typename T>
struct t_type_info : type_info {};

template <typename T>
t_type_info<T> make_type_info(std::string_view name)
{
    t_type_info<T> ret;
    ret.name = name;
    set_missing_traits_to_info<T>(ret);
    return ret;
}

} // namespace trex
