/**
 * @file process_termination.hpp
 * @brief Process termination management.
 */

#ifndef FCPP_PROCESS_TERMINATION_H_
#define FCPP_PROCESS_TERMINATION_H_

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/option/distribution.hpp"

#include "lib/generals.hpp"
#include "lib/common_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Generating distribution for distance estimations.
std::weibull_distribution<real_t> dist_distr = distribution::make<std::weibull_distribution>(real_t(1), real_t(dist_dev*0.01));


//! @brief Adjusted nbr_dist value accounting for errors.
FUN field<real_t> adjusted_nbr_dist(ARGS) {
    return node.nbr_dist() * rand_hood(CALL, dist_distr) + node.storage(tags::speed{}) * comm / period * node.nbr_lag();
}

//! @brief Message bytes overhead per process due to the termination strategy.
template <typename T>
class termination_overhead;

//! @brief Legacy termination logic (COORD19).
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::legacy>) {
     bool terminating = s == status::terminated_output;
     bool terminated = old(CALL, terminating, [&](bool ot){
        return any_hood(CALL, nbr(CALL, ot), ot) or terminating;
     });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);
    if (exiting) s = status::external_deprecated;
    else if (terminating) s = status::internal_output;
}
//! @brief Overhead of the legacy termination logic.
template<template<class> class T> class termination_overhead<T<tags::legacy>> : public std::integral_constant<size_t, 2*sizeof(bool) + 2*sizeof(trace_t)> {};

//! @brief Legacy termination logic updated to use share (LMCS2020) instead of rep+nbr.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::share>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);
    if (exiting) s = status::external_deprecated;
    else if (terminating) s = status::internal_output;
}
//! @brief Overhead of the share termination logic.
template<template<class> class T> class termination_overhead<T<tags::share>> : public std::integral_constant<size_t, 2*sizeof(bool) + 2*sizeof(trace_t)> {};

//! @brief Novel termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::ispp>) {
    using namespace tags;

    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid;
    double ds = monotonic_distance(CALL, source, adjusted_nbr_dist(CALL));
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
        if (s == status::internal_output) s = status::border_output;
    }
}
//! @brief Overhead of the novel termination logic.
template<template<class> class T> class termination_overhead<T<tags::ispp>> : public std::integral_constant<size_t, sizeof(bool) + 2*sizeof(real_t) + 3*sizeof(trace_t)> {};

//! @brief Wave-like termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::wispp>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid and old(CALL, true, false);
    double ds = monotonic_distance(CALL, source, adjusted_nbr_dist(CALL));
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;        
        if (s == status::internal_output) s = status::border_output;
    }
}
//! @brief Overhead of the wave-like termination logic.
template<template<class> class T> class termination_overhead<T<tags::wispp>> : public std::integral_constant<size_t, sizeof(bool) + 2*sizeof(real_t) + 3*sizeof(trace_t)> {};

//! @brief Export list for termination_logic.
FUN_EXPORT termination_logic_t = export_list<bool, monotonic_distance_t>;

//! @brief Result type of spawn calls with messages as keys.
using message_log_type = std::unordered_map<message, times_t, common::hash<message>>;

//! @brief Result type of spawn calls.
using key_log_type = std::unordered_map<device_t, message, common::hash<device_t>>;


//! @brief Computes stats on message delivery and active processes.
GEN(T) void proc_stats(ARGS, message_log_type const& nm, int render, T, size_t base_overhead, size_t variable_overhead) {
    // import tags for convenience
    using namespace tags;
    // stats on number of active processes
    int proc_num = node.storage(proc_data{}).size() - 1;
    node.storage(max_proc<T>{}) = max(node.storage(max_proc<T>{}), proc_num);
    node.storage(tot_proc<T>{}) += proc_num;
    size_t ms = proc_num * (sizeof(message) + sizeof(status) + termination_overhead<T>::value + variable_overhead) + base_overhead;
    node.storage(max_msg_size<T>{}) = max(node.storage(max_msg_size<T>{}), ms);
    node.storage(tot_msg_size<T>{}) += ms;
    // additional node rendering
    if (render >= 0) {
        if (proc_num > 0) node.storage(node_size{}) *= 1.2;
        if (render == 0) node.storage(node_color{})  = node.storage(proc_data{}).back();
        if (render == 1) node.storage(left_color{})  = node.storage(proc_data{}).back();
        if (render == 2) node.storage(right_color{}) = node.storage(proc_data{}).back();
    }
    // stats on delivery success
    old(node, call_point, message_log_type{}, [&](message_log_type m){
        for (auto const& x : nm) {
            if (m.count(x.first)) {
                node.storage(repeat_count<T>{}) += 1;
            } else {
                node.storage(first_delivery_tot<T>{}) += x.second - x.first.time;
                node.storage(delivery_count<T>{}) += 1;
                m[x.first] = x.second;
            }
        }
        return m;
    });
}
//! @brief Export list for proc_stats.
FUN_EXPORT proc_stats_t = export_list<message_log_type>;

//! @brief Wrapper calling a spawn function with a given process and key set, while tracking the processes executed.
GEN(T,G,S) message_log_type spawn_profiler(ARGS, T, G&& process, S&& key_set, real_t v, int render, size_t base_overhead, size_t variable_overhead) {
    // dispatches messages
    message_log_type r = spawn_deprecated(node, call_point, [&](message const& m){
        auto r = process(m);
        termination_logic(CALL, get<1>(r), v, m, T{});
        real_t key = get<1>(r) == status::external_deprecated ? 0.5 : 1;
        node.storage(tags::proc_data{}).push_back(color::hsva(m.data * 360, key, key));
        return r;
    }, std::forward<S>(key_set));
    // compute stats
    proc_stats(CALL, r, render, T{}, base_overhead, variable_overhead);

    return r;
}
//! @brief Export list for spawn_profiler.
FUN_EXPORT spawn_profiler_t = export_list<spawn_t<message, status>, termination_logic_t, proc_stats_t>;

} // coordination

} // fcpp

#endif // // FCPP_PROCESS_TERMINATION_H_
