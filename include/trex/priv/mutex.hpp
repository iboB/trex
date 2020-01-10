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
