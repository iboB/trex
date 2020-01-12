// T-Rex
// Copyright (c) 2020 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <cstring>
#include <cstddef>

namespace trex
{
namespace priv
{

// C++17: remove and switch to string_view
class cstr_view
{
public:
    cstr_view() = default;
    cstr_view(const char* begin, const char* end)
        : m_begin(begin), m_end(end)
    {}
    cstr_view(const char* begin, size_t length)
        : cstr_view(begin, begin + length)
    {}
    cstr_view(const char* str)
        : cstr_view(str, std::strlen(str))
    {}

    explicit operator bool() const { return m_begin != m_end; }

    size_t length() const { return m_end - m_begin; }
    const char* data() const { return m_begin; }

    const char* begin() const { return m_begin; }
    const char* end() const { return m_end; }

private:
    const char* m_begin = nullptr;
    const char* m_end = nullptr;
};

bool operator==(const cstr_view& a, const cstr_view& b)
{
    // C++14: use std::equal
    const auto la = a.length();
    const auto lb = b.length();
    if (la != lb) return false;
    for (auto pa = a.begin(), pb = b.begin(); pa != a.end(); ++pa, ++pb)
    {
        if (*pa != *pb) return false;
    }
    return true;
}

bool operator!=(const cstr_view& a, const cstr_view& b)
{
    return !operator==(a, b);
}

} // namespace priv
} // namespace trex
