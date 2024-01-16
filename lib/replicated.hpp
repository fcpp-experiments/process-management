// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file replicated.hpp
 * @brief Generic algorithm replicator, applied to Past-CTL temporal logic operators.
 */

#ifndef FCPP_REPLICATED_H_
#define FCPP_REPLICATED_H_

#include "lib/past_ctl.hpp"
#include "lib/slcs.hpp"
#include "lib/coordination/time.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

namespace tags {
    struct critic {};
    struct ever_critic {};
    struct now_critic__SLCS {};
    struct now_critic__replicated {};
}


/**
 * Generic algorithm replicator, returning the value of the oldest
 * replica currently running.
 *
 * @param n   The number of replicas.
 * @param t   The interval between replica spawning.
 * @param fun The aggregate code to replicate.
 * @param xs  Arguments for the aggregate code.
 */
GEN(F, ... Ts) auto replicate(ARGS, size_t n, times_t t, F fun, Ts const&... xs) { CODE
    size_t now = shared_clock(CALL) / t;
    auto res = spawn(CALL, [&](size_t i){
        return make_tuple(fun(CALL, xs...), i > now - n);
    }, common::option<size_t, true>{now});
    for (auto x : res) if (x.first > now - n) now = min(now, x.first);
    return res[now];
}
//! @brief Export list for replicate.
FUN_EXPORT replicate_t = export_list<spawn_t<size_t, bool>, shared_clock_t>;


//! @brief Finally/somewhere operator, implemented by replicating .
FUN bool somewhere(ARGS, bool f, size_t replicas, real_t diameter, real_t infospeed) { CODE
    return replicate(CALL, replicas, diameter / infospeed / (replicas-1), logic::EP, f);
}
//! @brief Export list for somewhere.
FUN_EXPORT somewhere_t = export_list<replicate_t, past_ctl_t>;


//! @brief Case study checking whether a critic event is happening.
FUN void criticality_control(ARGS) {
    using namespace tags;
    bool c = node.uid == 0 and node.current_time() > 10 and node.current_time() < 15;
    node.storage(critic{}) = c;
    node.storage(ever_critic{}) = logic::EP(CALL, c);
    node.storage(now_critic__SLCS{}) = logic::F(CALL, c);
    node.storage(now_critic__replicated{}) = somewhere(CALL, c, 4, node.storage(diameter{})*node.storage(radius{}), node.storage(radius{})/node.storage(period{}));
}
//! @brief Export list for criticality_control.
FUN_EXPORT criticality_control_t = export_list<somewhere_t>;

}

}

#endif // FCPP_REPLICATED_H_
