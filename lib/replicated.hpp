// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file replicated.hpp
 * @brief Generic algorithm replicator, applied to Past-CTL temporal logic operators.
 */

#ifndef FCPP_REPLICATED_H_
#define FCPP_REPLICATED_H_

#include "lib/past_ctl.hpp"
#include "lib/coordination/time.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

/**
 * Generic algorithm replicator, returning the value of the oldest
 * replica currently running.
 *
 * @param fun The aggregate code to replicate.
 * @param n   The number of replicas.
 * @param t   The interval between replica spawning.
 * @param xs  Arguments for the aggregate code.
 */
GEN(F, ... Ts) auto replicate(ARGS, F fun, size_t n, times_t t, Ts const&... xs) { CODE
    size_t now = shared_clock(CALL) / t;
    auto res = spawn(CALL, [&](size_t i){
        return make_tuple(fun(CALL, xs...), i > now - n);
    }, common::option<size_t, true>{now});
    for (auto x : res) now = min(now, x.first);
    return res[now];
}
//! @brief Export list for replicate.
FUN_EXPORT replicate_t = export_list<spawn_t<size_t, bool>, shared_clock_t>;


//! @brief Finally/somewhere operator, implemented by replicating .
FUN bool somewhere(ARGS, bool f, size_t replicas, real_t diameter, real_t infospeed) { CODE
    return replicate(CALL, logic::EP, replicas, diameter / infospeed / (replicas-1), f);
}
FUN_EXPORT somewhere_t = export_list<replicate_t, past_ctl_t>;

}

}

#endif // FCPP_REPLICATED_H_
