// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file message_dispatch.hpp
 * @brief Aggregate process dispatching point-to-point messages, avoiding to flood the network.
 */

#ifndef FCPP_MESSAGE_DISPATCH_H_
#define FCPP_MESSAGE_DISPATCH_H_

#include "lib/beautify.hpp"
#include "lib/coordination.hpp"
#include "lib/data.hpp"

#include "process_utils.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


namespace tags {
    //! @brief Tree process.
    struct tree {};
    
    //! @brief Distance to the central node.
    struct center_dist {};
}

using set_t = std::unordered_set<device_t>;
using map_t = std::unordered_map<message, times_t>;

//! @brief Main function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), node.storage(speed{}), 1);
    device_t src_id = 0;
    // distance estimation
    bool is_src = node.uid == src_id;
    double ds = bis_distance(CALL, is_src, 1, 100);
    // basic node rendering
    node.storage(center_dist{}) = ds;
    node.storage(node_color{}) = color::hsva(ds*hue_scale, 1, 1);
    node.storage(node_shape{}) = is_src ? shape::cube : shape::icosahedron;
    node.storage(node_size{}) = is_src ? 16 : 10;
    // spanning tree definition
    device_t parent = get<1>(min_hood(CALL, make_tuple(nbr(CALL, ds), node.nbr_uid())));
    // routing sets along the tree
    set_t below = sp_collection(CALL, ds, set_t{node.uid}, set_t{}, [](set_t x, set_t const& y){
        x.insert(y.begin(), y.end());
        return x;
    });
    // random message with 1% probability during time [10..50]
    common::option<message> m;
    if (node.current_time() > 10 and node.current_time() < 50 and node.next_real() < 0.01) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time());
        node.storage(sent_count<tree>{}) += 1;
    }
    // dispatches messages
    std::vector<color> procs{color(BLACK)};
    map_t r = spawn(CALL, [&](message const& m){
        procs.push_back(color::hsva(m.to*360.0/devices, 1, 1));
        bool inpath = below.count(m.from) + below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   inpath ? status::internal : status::external;
        return make_tuple(node.current_time(), s);
    }, m);
    // process and msg stats
    node.storage(max_proc<tree>{}) = max(node.storage(max_proc<tree>{}), procs.size() - 1);
    node.storage(tot_proc<tree>{}) += procs.size() - 1;
    if (procs.size() > 1) node.storage(node_size{}) *= 1.5;
    // additional node rendering
    node.storage(left_color{})  = procs[min(int(procs.size()), 2)-1];
    node.storage(right_color{}) = procs[min(int(procs.size()), 3)-1];
    // persist received messages and delivery stats
    r = old(CALL, map_t{}, [&](map_t m){
        for (auto const& x : r) {
            if (m.count(x.first)) node.storage(repeat_count<tree>{}) += 1;
            else {
                node.storage(first_delivery<tree>{}) += x.second - x.first.time;
                node.storage(delivery_count<tree>{}) += 1;
                m[x.first] = x.second;
            }
        }
        return m;
    });
}
//! @brief Exports for the main function.
FUN_EXPORT main_t = export_list<rectangle_walk_t<3>, bis_distance_t, sp_collection_t<double, set_t>, device_t, spawn_t<message, status>, map_t>;


}


}

#endif // FCPP_MESSAGE_DISPATCH_H_
