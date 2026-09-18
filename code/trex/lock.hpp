// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <mutex>

namespace trex::lock {

struct thread_safe {
    using mutex = std::mutex;
    using lock_guard = std::lock_guard<mutex>;
};

struct fast {
    struct mutex {
        void lock() {}
        void unlock() {}
    };
    struct lock_guard {
        lock_guard(mutex&) {};
    };
};

using default_lock = fast;

} // namespace trex::lock
