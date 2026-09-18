// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <mutex>

namespace trex::reg {

struct thread_safe_registration {
    using mutex = std::mutex;
    using lock_guard = std::lock_guard<mutex>;
};

struct fast_registration {
    struct mutex {
        void lock() {}
        void unlock() {}
    };
    struct lock_guard {
        lock_guard(mutex&) {};
    };
};

using default_registration = fast_registration;

} // namespace trex::reg
