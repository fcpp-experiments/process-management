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
    struct now_critic_SLCS {};
    struct now_critic_replicated {};
    struct comm_rad {};
    struct period {};
}


/**
 * Generic algorithm replicator, returning the value of the oldest
 * replica currently running.
 *
 * @param fun The aggregate code to replicate (without arguments).
 * @param n   The number of replicas.
 * @param t   The interval between replica spawning.
 */
GEN(F) auto replicate(ARGS, F fun, size_t n, times_t t) { CODE
    size_t now = shared_clock(CALL) / t;
    auto res = spawn(CALL, [&](size_t i){
        return make_tuple(fun(), i > now - n);
    }, common::option<size_t, true>{now});
    for (auto x : res) if (x.first > now - n) now = min(now, x.first);
    return res[now];
}
//! @brief Export list for replicate.
FUN_EXPORT replicate_t = export_list<spawn_t<size_t, bool>, shared_clock_t>;


//! @brief Finally/somewhere operator, implemented by replicating .
FUN bool somewhere(ARGS, bool f, size_t replicas, real_t diameter, real_t infospeed) { CODE
    return replicate(CALL, [&](){
        return logic::EP(CALL, f);
    }, replicas, diameter / infospeed / (replicas-1));
}
//! @brief Export list for somewhere.
FUN_EXPORT somewhere_t = export_list<replicate_t, past_ctl_t>;


//! @brief Case study checking whether a critic event is happening.
FUN void criticality_control(ARGS) {
    using namespace tags;
    bool c = node.uid == 42 and node.current_time() > 10 and node.current_time() < 15;
    node.storage(critic{}) = c;
    node.storage(ever_critic{}) = logic::EP(CALL, c);
    node.storage(now_critic_SLCS{}) = logic::F(CALL, c);
    node.storage(now_critic_replicated{}) = somewhere(CALL, c, 4, node.storage(diameter{})*node.storage(comm_rad{}), node.storage(comm_rad{})/node.storage(period{}));
}
//! @brief Export list for criticality_control.
FUN_EXPORT criticality_control_t = export_list<somewhere_t, slcs_t>;

}

}

#endif // FCPP_REPLICATED_H_
