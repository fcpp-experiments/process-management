// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file xc_processes.hpp
 * @brief Case study on XC processes.
 */

#ifndef FCPP_XC_PROCESSES_H_
#define FCPP_XC_PROCESSES_H_

#include <iostream>

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/generals.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Handles a process, spawning instances of it for every key in the `key_set` and passing general arguments `xs` (overload with field<bool> status).
//! @brief Handles a process, spawning instances of it for every key in the `key_set` and passing general arguments `xs` (overload with field<bool> status).
template <typename node_t, typename G, typename S, typename... Ts, typename K = typename std::decay_t<S>::value_type, typename T = std::decay_t<std::result_of_t<G(K const&, Ts const&...)>>, typename R = std::decay_t<tuple_element_t<0,T>>, typename B = std::decay_t<tuple_element_t<1,T>>>
std::enable_if_t<std::is_same<B,field<bool>>::value, std::unordered_map<K, R>>
spawn(node_t& node, trace_t call_point, G&& process, S&& key_set, Ts const&... xs) {
    return spawn(node, call_point, [&](K const& k, auto const&... params){
        return nbr(node, call_point, field<bool>(false), [&](field<bool> n){
            bool b = false;
            R ret;
            field<bool> fb = false;

            bool found=false;
            for (auto it = key_set.cbegin(); it < key_set.cend(); it++) {
                if (*it == k) {
                    found = true;
                    break;
                }
            }

            if (coordination::any_hood(node, call_point, n) or found) {
                tie(ret, fb) = process(k, params...);
                b = coordination::any_hood(node, call_point, fb) or other(fb);
            }
            return make_tuple(make_tuple(ret, b), fb);
        });
    }, std::forward<S>(key_set), xs...);
}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;


//! @brief Possibly generates a message, given the number of devices and the experiment tag.
FUN common::option<message> get_message(ARGS, size_t devices) {
    common::option<message> m;
    #ifndef MULTI_TEST
    bool genmsg = node.uid == devices-1 && node.current_time() > 10 && node.storage(tags::sent_count{}) == 0;
    #else
    bool genmsg = node.uid >= devices-10 && node.current_time() > 1 && node.current_time() < 26 && node.next_real() < 0.05;
    #endif
    // random message with 1% probability during time [10..50]
    if (genmsg) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time(), node.next_real());
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}

//! @brief Result type of spawn calls dispatching messages.
// TODO ****check --> should be size_t
using message_log_type = std::unordered_map<message, double>;
//using message_log_type = std::unordered_map<message, bool>;

//! @brief Computes stats on message delivery and active processes.
GEN(T) void proc_stats(ARGS, message_log_type const& nm, bool render, T) {
    // import tags for convenience
    using namespace tags;
    // stats on number of active processes
    int proc_num = node.storage(proc_data{}).size() - 1;
#ifdef ALLPLOTS
    node.storage(max_proc<T>{}) = max(node.storage(max_proc<T>{}), proc_num);
#endif
    node.storage(tot_proc<T>{}) += proc_num;
    // additional node rendering
    if (render) {
        if (proc_num > 0) node.storage(node_size{}) *= 1.5;
        node.storage(node_color{})  = node.storage(proc_data{})[min(proc_num, 1)];
        node.storage(left_color{})  = node.storage(proc_data{})[min(proc_num, 2)];
        node.storage(right_color{}) = node.storage(proc_data{})[min(proc_num, 3)];
    }
    // stats on delivery success
    old(node, call_point, message_log_type{}, [&](message_log_type m){
        for (auto const& x : nm) {
            if (m.count(x.first)) {
#ifdef ALLPLOTS
                node.storage(repeat_count<T>{}) += 1;
#endif
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
GEN(T,G,S) void spawn_profiler(ARGS, T, G&& process, S&& key_set, real_t v, bool render) {
    // clear up stats data
    node.storage(tags::proc_data{}).clear();
    node.storage(tags::proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));
    // dispatches messages
    message_log_type r = spawn(node, call_point, [&](message const& m){
        auto r = process(m, v);
        // TODO **** adapt to field<bool>
        //real_t key = get<0>(r) == status::external ? 0.5 : 1;
        real_t key = get<0>(r) ? 0.5 : 1;
        //real_t key = 1;
        node.storage(tags::proc_data{}).push_back(color::hsva(m.data * 360, key, key));
        return r;
    }, std::forward<S>(key_set));

    // compute stats
    proc_stats(CALL, r, render, T{});
}
//! @brief Export list for spawn_profiler.
FUN_EXPORT spawn_profiler_t = export_list<spawn_t<message, bool>, proc_stats_t, field<bool>>;

//! @brief Makes test for spherical processes.
GEN(T) void spherical_test(ARGS, common::option<message> const& m, T, bool render = false) { CODE
    #ifdef ALG_INFOSPEED
    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m, real_t v){
        bool source = m.from == node.uid and old(CALL, true, false);
        double ds = monotonic_distance(CALL, source, node.nbr_dist());
        double dt = monotonic_distance(CALL, source, node.nbr_lag());

        field<real_t> fdds = nbr(CALL, ds);
        //field<real_t> fddt = nbr(CALL, dt);
        field<real_t> fddt = dt + period - node.nbr_lag();

        field<bool> fdnslow = (fdds >= v * comm / period * (fddt - node.nbr_lag()));
        fdnslow = mod_other(CALL, fdnslow, true);
        if (node.uid == m.to)
            fdnslow = mod_self(CALL, fdnslow, false);

        return make_tuple(node.current_time(), fdnslow);

    }, m, node.storage(tags::infospeed{}), render);
    #else
    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m, real_t v){
        bool source = m.from == node.uid;
        double dt = monotonic_distance(CALL, source, node.nbr_lag());
        field<real_t> fddt = nbr(CALL, dt);

        field<bool> fdwav;
        
        fdwav = (fddt >= dt);
        fdwav = mod_self(CALL, fdwav, false);

        bool dest = m.to == node.uid;
        int rnd = counter(CALL);

        if (dest) {
            fdwav = field<bool>(false);
        } else if (rnd == 1) {
            fdwav = field<bool>(false);
            fdwav = mod_self(CALL, fdwav, true);
            fdwav = mod_other(CALL, fdwav, true);
        } else {
             fdwav = field<bool>(false);
        }

        return make_tuple(node.current_time(), fdwav);

    }, m, node.storage(tags::infospeed{}), render);
    #endif
}
FUN_EXPORT spherical_test_t = export_list<spawn_profiler_t, double, monotonic_distance_t, bool, int>;

//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}) * comm / period, 1);

    bool is_src = false;
    //bool is_src = node.uid == 0;

    bool highlight = is_src or node.uid == node.storage(devices{}) - 1;
    node.storage(node_shape{}) = is_src ? shape::icosahedron : highlight ? shape::cube : shape::sphere;
    node.storage(node_size{}) = highlight ? 20 : 10;
    // random message with 1% probability during time [10..50]
    common::option<message> m = get_message(CALL, node.storage(devices{}));

    spherical_test(CALL, m, xc{}, true);
}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, spherical_test_t, flex_parent_t, real_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
