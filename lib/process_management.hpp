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

//! @brief The root of the communication tree in the network.
constexpr device_t message_root = 0;

//! @brief The device sending the message.
constexpr device_t message_sender = 1;

//! @brief The device receiving the message.
constexpr device_t message_receiver = 2;


//! @brief Possibly generates a message, given the number of devices and the experiment tag.
FUN common::option<message> get_message(ARGS) {
    common::option<message> m;
    if (node.uid == message_sender && node.current_time() > 10 && node.storage(tags::sent_count{}) == 0) {
        m.emplace(node.uid, message_receiver, node.current_time(), node.next_real());
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}


//! @brief Message bytes overhead fixed and per process due to the propagation shape.
template <template<class> class T>
class topological_overhead;

//! @brief Makes test for spherical processes.
GEN(T) void spherical_test(ARGS, common::option<message> const& m, T, int render = -1) { CODE
    // clear up stats data
    node.storage(tags::proc_data{}).clear();
    node.storage(tags::proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));

    spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m){
        status s = node.uid == m.to ? status::terminated_output : status::internal;
        return make_tuple(node.current_time(), s);
    }, m, 2.5, render, 0, 0);
}
FUN_EXPORT spherical_test_t = export_list<spawn_profiler_t>;


//! @brief Makes test for tree processes.
GEN(T,S) void tree_test(ARGS, common::option<message> const& m, device_t parent, S const& below, size_t set_size, T, int render = -1) { CODE
    // clear up stats data
    node.storage(tags::proc_data{}).clear();
    node.storage(tags::proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));

    spawn_profiler(CALL, tags::tree<T>{}, [&](message const& m){
        bool source_path = any_hood(CALL, nbr(CALL, parent) == node.uid) or node.uid == m.from;
        bool dest_path = below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   source_path or dest_path ? status::internal : status::external_deprecated;
        return make_tuple(node.current_time(), s);
    }, m, 0.3, render, set_size + 2*sizeof(trace_t) + sizeof(real_t) + sizeof(device_t), sizeof(trace_t));
}
//! @brief Exports for the main function.
FUN_EXPORT tree_test_t = export_list<spawn_profiler_t>;


#ifdef BLOOM
//! @brief The type for a set of devices.
using set_t = bloom_filter<2,256>;
#else
//! @brief The type for a set of devices.
using set_t = std::unordered_set<device_t>;
#endif

//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // basic node rendering
    #ifdef NOTREE
        bool is_src = false;
    #else
        bool is_src = node.uid == message_root;
    #endif

    bool highlight = is_src or node.uid == message_sender or node.uid == message_receiver;
    node.storage(node_shape{}) = is_src ? shape::star : node.uid == message_receiver ? shape::icosahedron : highlight ? shape::cube : shape::sphere;
    node.storage(node_size{}) = is_src ? 30 : highlight ? 20 : 10;
    // random walk
    size_t l = node.storage(side{});
    if (highlight) {
        if (is_src) node.position() = make_vec(l/2, l/2, 20);
        if (node.uid == message_sender) node.position() = make_vec(l/4, l/4, 20);
        if (node.uid == message_receiver) node.position() = make_vec(3*l/4, 3*l/4, 20);
    } else rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}) * comm / period, 1);
    // standard message from message_sender to message_receiver after time 10
    common::option<message> m = get_message(CALL);
#ifndef NOSPHERE
    // tests spherical processes with legacy termination
    spherical_test(CALL, m, legacy{});
    spherical_test(CALL, m, share{}, 0); // central color
    spherical_test(CALL, m, ispp{},  1); // left color
    spherical_test(CALL, m, wispp{}, 2); // right color
#endif
#ifndef NOTREE
    // spanning tree definition
    device_t parent = flex_parent(CALL, is_src, comm);
    // routing sets along the tree
    set_t below = parent_collection(CALL, parent, set_t{node.uid}, [](set_t x, set_t const& y){
#ifdef BLOOM
        x.insert(y);
#else
        x.insert(y.begin(), y.end());
#endif
        return x;
    });
    common::osstream os;
    os << below;
    // test tree processes with legacy termination
    tree_test(CALL, m, parent, below, os.size(), legacy{});
    tree_test(CALL, m, parent, below, os.size(), share{}, 0); // central color
    tree_test(CALL, m, parent, below, os.size(), ispp{},  1); // left color
    tree_test(CALL, m, parent, below, os.size(), wispp{}, 2); // right color
#endif
}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, spherical_test_t, flex_parent_t, real_t, parent_collection_t<set_t>, tree_test_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
