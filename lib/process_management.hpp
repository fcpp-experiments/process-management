// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file process_management.hpp
 * @brief Case study on process lifetime management.
 */

#ifndef FCPP_PROCESS_MANAGEMENT_H_
#define FCPP_PROCESS_MANAGEMENT_H_

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/option/distribution.hpp"

#include "lib/generals.hpp"
#include "lib/termination.hpp"
#include "lib/simulation_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Handles a process, spawning instances of it for every key in the `key_set` and passing general arguments `xs` (overload with field<bool> status).
template <typename node_t, typename G, typename S, typename... Ts, typename K = typename std::decay_t<S>::value_type, typename T = std::decay_t<std::result_of_t<G(K const&, Ts const&...)>>, typename R = std::decay_t<tuple_element_t<0,T>>, typename B = std::decay_t<tuple_element_t<1,T>>>
std::enable_if_t<std::is_same<B,field<bool>>::value, std::unordered_map<K, R>>
spawn(node_t& node, trace_t call_point, G&& process, S&& key_set, Ts const&... xs) {
    return spawn(node, call_point, [&](K const& k, auto const&... params){
        return nbr(node, call_point, field<bool>(false), [&](field<bool> n){
            bool b = false;
            field<bool> fb = false;
            if (any_hood(node, call_point, n) or key_set.count(k) > 0) {
                fb = process(k, params...);
                b = any_hood(node, call_point, fb) or other(fb);
            }
            return make_tuple(b, fb);
        });
    }, std::forward<S>(key_set), xs...);
}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Possibly generates a message, given the number of devices and the experiment tag.
FUN common::option<message> get_message(ARGS, size_t devices) {
    common::option<message> m;
    // random message with 1% probability during time [10..50]
    if (node.uid == devices-1 && node.current_time() > 10 && node.storage(tags::sent_count{}) == 0) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time(), node.next_real());
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}


//! @brief Makes test for spherical processes.
GEN(T) void spherical_test(ARGS, common::option<message> const& m, T, bool render = false) { CODE
    // clear up stats data
    node.storage(proc_data{}).clear();
    node.storage(proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));

    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m){
        status s = node.uid == m.to ? status::terminated_output : status::internal;
        return make_tuple(node.current_time(), s);
    }, m, node.storage(tags::infospeed{}), render);
}
FUN_EXPORT spherical_test_t = export_list<spawn_profiler_t>;


//! @brief The type for a set of devices.
using set_t = std::unordered_set<device_t>;

//! @brief Makes test for tree processes.
GEN(T) void tree_test(ARGS, common::option<message> const& m, device_t parent, set_t const& below, T, bool render = false) { CODE
    // clear up stats data
    node.storage(proc_data{}).clear();
    node.storage(proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));

    spawn_profiler(CALL, tags::tree<T>{}, [&](message const& m){
        bool source_path = any_hood(CALL, nbr(CALL, parent) == node.uid) or node.uid == m.from;
        bool dest_path = below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   source_path or dest_path ? status::internal : status::external;
        return make_tuple(node.current_time(), s);
    }, m, 0.9, render);
}
//! @brief Exports for the main function.
FUN_EXPORT tree_test_t = export_list<spawn_profiler_t>;


//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}) * comm / period, 1);
    // basic node rendering
#ifdef NOTREE
    bool is_src = false;
#else
    bool is_src = node.uid == 0;
#endif
    bool highlight = is_src or node.uid == node.storage(devices{}) - 1;
    node.storage(node_shape{}) = is_src ? shape::icosahedron : highlight ? shape::cube : shape::sphere;
    node.storage(node_size{}) = highlight ? 20 : 10;
    // random message with 1% probability during time [10..50]
    common::option<message> m = get_message(CALL, node.storage(devices{}));
#ifndef NOSPHERE
    // tests spherical processes with legacy termination
    spherical_test(CALL, m, legacy{});
    spherical_test(CALL, m, share{}, true);
    spherical_test(CALL, m, ispp{});
    spherical_test(CALL, m, wispp{});
#endif
#ifndef NOTREE
    // spanning tree definition
    device_t parent = flex_parent(CALL, is_src, comm);
    // routing sets along the tree
    set_t below = parent_collection(CALL, parent, set_t{node.uid}, [](set_t x, set_t const& y){
        x.insert(y.begin(), y.end());
        return x;
    });
    // test tree processes with legacy termination
    tree_test(CALL, m, parent, below, legacy{});
    tree_test(CALL, m, parent, below, share{});
    tree_test(CALL, m, parent, below, ispp{});
    tree_test(CALL, m, parent, below, wispp{});
#endif
}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, spherical_test_t, flex_parent_t, real_t, parent_collection_t<set_t>, tree_test_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
