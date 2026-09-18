// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include "facet_id.hpp"

namespace trex {

template <typename Domain>
Domain& get_facet_domain() {
    return _trex_get_facet_domain(static_cast<Domain*>(nullptr));
}

template <typename Domain, typename Facet>
facet_id get_facet_id() {
    return _trex_get_facet_id(static_cast<Domain*>(nullptr), static_cast<Facet*>(nullptr));
}

} // namespace trex
