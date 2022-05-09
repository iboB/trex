// T-Rex
// Copyright (c) 2020-2022 Borislav Stanimirov
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

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

} // namespace priv
} // namespace trex
