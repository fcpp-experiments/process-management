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


//! @brief Max distance of a broadcast
// TODO should be something like this:
//constexpr double max_distance = side / 2.0;
constexpr double max_distance = INF;


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


namespace tags {
    //! @brief The spherical process
    struct spherical {};
    
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
    bool is_src = node.uid == src_id;

    // basic node rendering
    node.storage(node_shape{}) = is_src ? shape::cube : shape::icosahedron;
    node.storage(node_size{}) = is_src ? 16 : 10;
	
    // random message with 1% probability during time [10..50]
    common::option<message> m;
    // TODO should be something like this:
    /*
    if (node.current_time() > 10 and node.current_time() < 50 and node.next_real() < 0.01) {
        m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time());
        node.storage(sent_count{}) += 1;
    }
    */
    if (is_src && node.current_time() > 3 && node.current_time() < 4) {
	// TODO should be
	//  m.emplace(node.uid, (device_t)node.next_int(devices-1), node.current_time());
	m.emplace(node.uid, 82, node.current_time());

        node.storage(sent_count<spherical>{}) += 1;	
    }
    
    // dispatches messages
    std::vector<color> procs{color(BLACK)};
    std::vector<double> procs_dist{0};
    map_t r = spawn_legacy(CALL, [&](message const& m){
	// TODO source of the process?
	//	bool is_src = node.uid == m.from;
		
        procs.push_back(color::hsva(m.to*360.0/devices, 1, 1));

	double ds = bis_distance(CALL, is_src, 1, 100);
	procs_dist.push_back(ds);
	
	bool inpath = ds < max_distance;

	// TODO should be something like
	status s = node.uid == m.to ? status::terminated_output :
	    inpath ? status::internal : status::external;
	/* no termination
	status s = inpath ? status::internal : status::external;
	*/
        return make_tuple(node.current_time(), s);	
    }, m);

    size_t dsidx = max(int(procs_dist.size()) - 1, 0);
    node.storage(center_dist{}) = procs_dist[dsidx];
    node.storage(node_color{}) = color::hsva(procs_dist[dsidx]*hue_scale, 1, 1);
    
    // process and msg stats
    node.storage(max_proc<spherical>{}) = max(node.storage(max_proc<spherical>{}), procs.size() - 1);
    node.storage(tot_proc<spherical>{}) += procs.size() - 1;
    if (procs.size() > 1) node.storage(node_size{}) *= 1.5;

    // additional node rendering
    node.storage(left_color{})  = procs[min(int(procs.size()), 2)-1];
    node.storage(right_color{}) = procs[min(int(procs.size()), 3)-1];

    // persist received messages and delivery stats
    r = old(CALL, map_t{}, [&](map_t m){
        for (auto const& x : r) {
            if (m.count(x.first)) node.storage(repeat_count<spherical>{}) += 1;
            else {
                node.storage(first_delivery<spherical>{}) += x.second - x.first.time;
                node.storage(delivery_count<spherical>{}) += 1;
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

#endif // FCPP_SPHERICAL_PROCESS_H_
