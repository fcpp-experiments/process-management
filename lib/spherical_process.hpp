// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file spherical_process.hpp
 * @brief Aggregate process TODO.
 */

#ifndef FCPP_SPHERICAL_PROCESS_H_
#define FCPP_SPHERICAL_PROCESS_H_

#include "lib/beautify.hpp"
#include "lib/coordination.hpp"
#include "lib/data.hpp"


#include "process_utils.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Minimum number whose square is at least n.
constexpr size_t discrete_sqrt(size_t n) {
    size_t lo = 0, hi = n, mid = 0;
    while (lo < hi) {
        mid = (lo + hi)/2;
        if (mid*mid < n) lo = mid+1;
        else hi = mid;
    }
    return lo;
}

//! @brief Number of devices.
constexpr size_t devices = 300;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Side of the deployment area.
constexpr size_t side = discrete_sqrt(devices * 3000);

//! @brief Height of the deployment area.
constexpr size_t height = 100;


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


//! @brief Main function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    rectangle_walk(CALL, make_vec(0,0,0), make_vec(side,side,height), node.storage(speed{}), 1);
    // basic node rendering
    bool is_src = node.uid == 0;
    node.storage(node_shape{}) = is_src ? shape::cube : shape::sphere;
    node.storage(node_size{}) = is_src ? 16 : 10;
    // random message with 1% probability during time [10..50]
    common::option<message> m = get_message(CALL, devices);
    // tests spherical processes with legacy termination
    spherical_test(CALL, m, INF, legacy{}, true);
    spherical_test(CALL, m, INF, share{});
    spherical_test(CALL, m, INF, novel{});
    spherical_test(CALL, m, INF, wave{});
    // spanning tree definition
    double ds = bis_distance(CALL, is_src, 1, 100);
    device_t parent = get<1>(min_hood(CALL, make_tuple(nbr(CALL, ds), node.nbr_uid())));
    // routing sets along the tree
    set_t below = sp_collection(CALL, ds, set_t{node.uid}, set_t{}, [](set_t x, set_t const& y){
        x.insert(y.begin(), y.end());
        return x;
    });
    // test tree processes with legacy termination
    tree_test(CALL, m, parent, below, legacy{});
    tree_test(CALL, m, parent, below, share{});
    tree_test(CALL, m, parent, below, novel{});
    tree_test(CALL, m, parent, below, wave{});
}
//! @brief Exports for the main function.
FUN_EXPORT main_t = export_list<rectangle_walk_t<3>, spherical_test_t, bis_distance_t, real_t, sp_collection_t<double, set_t>, tree_test_t>;


}


}

#endif // FCPP_SPHERICAL_PROCESS_H_
