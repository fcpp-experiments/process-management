// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file simulation_setup.hpp
 * @brief Simulation setup for the process management case study.
 */

#ifndef FCPP_CASE_STUDY_SETUP_H_
#define FCPP_CASE_STUDY_SETUP_H_

#include "lib/fcpp.hpp"
#include "lib/generals.hpp"

#include "lib/common_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

namespace coordination {
//! @brief Status of devices
enum devstatus
{
    IDLE,   // nothing interesting
    DISCO,  // discovery of service
    OFFER,  // offer of service
    SERVED, // being served
    SERVING // serving
};
}

//! @brief Namespace for component options.
namespace option {

//! @brief Storage for a given test.
template <template<class> class T, typename S>
using test_store_t = tuple_store<
    max_proc<T<S>>,            int,
    repeat_count<T<S>>,        size_t,
    max_msg_size<T<S>>,        size_t,
    tot_msg_size<T<S>>,        size_t,
    tot_proc<T<S>>,            int,
    first_delivery_tot<T<S>>,  times_t,
    delivery_count<T<S>>,      size_t
>;

template <int s, typename T = dev_status>
using status_aggregator = aggregator::filter<filter::equal<s>, aggregator::count<T>>;

using plot_t = plot::split<plot::time, 
   plot::join<plot::value<status_aggregator<coordination::devstatus::SERVING>>,
              plot::value<status_aggregator<coordination::devstatus::SERVED>>,
              plot::value<status_aggregator<coordination::devstatus::DISCO>>,
              plot::value<status_aggregator<coordination::devstatus::OFFER>>
   >  
>;

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN in process_management.hpp)
    exports<coordination::main_t>, // export type list (types used in messages)
    retain<metric::retain<2>>, // retain time for messages
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<log_s>, // the sequence generator for log events on the network
    spawn_schedule<sequence::multiple<i<devices, size_t>, n<0>>>, // the sequence generator of node creation events on the network
    // the basic contents of the node storage
    tuple_store<
        seed,                           uint_fast32_t,
        speed,                          double,
        devices,                        size_t,
        side,                           size_t,
        infospeed,                      double,
        proc_data,                      std::vector<color>,
        sent_count,                     size_t,
        node_color,                     color,
        left_color,                     color,
        right_color,                    color,
        node_size,                      double,
        node_shape,                     shape,
        num_svc_types,                  size_t,
        offered_svc,                    size_t,
        svc_rank,                       real_t,
        hops,                           size_t,
        // TODO: REMOVE
        best_rank,                      real_t,
        chosen_id,                      device_t,
        dev_status,                     coordination::devstatus
    >,
    // the basic tags and corresponding aggregators to be logged
    aggregators<
        sent_count,         aggregator::sum<size_t>,
        dev_status,         aggregator::combine<status_aggregator<coordination::devstatus::SERVING, double>,
                                                status_aggregator<coordination::devstatus::SERVED, double>,
                                                status_aggregator<coordination::devstatus::OFFER, double>,
                                                status_aggregator<coordination::devstatus::DISCO, double>>
    >,
    common::type_sequence<test_store_t<spherical,wispp>, test_store_t<tree,ispp>>,
    // data initialisation
    init<
        x,                  rectangle_d,
        seed,               functor::cast<distribution::interval_n<double, 0, seed_max>, uint_fast32_t>,
        infospeed,          i<infospeed>,
        speed,              functor::div<i<speed>, n<100>>,
        side,               i<side>,
        devices,            i<devices>,
        tvar,               functor::div<i<tvar>, n<100>>,
        tavg,               distribution::weibull<n<period>, functor::mul<i<tvar>, n<period, 100>>>,
        num_svc_types,      n<max_svc_id>,         
        offered_svc,        nu<max_svc_id>,
        svc_rank,           nu<1>,
        hops,               i<hops>
    >,
    // general parameters to use for plotting
    extra_info<
        tvar,   double,
        dens,   double,
        hops,   double,
        speed,  double
    >,
    plot_type<plot_t>, // the plot description to be used
    dimension<dim>, // dimensionality of the space
    connector<connect::fixed<comm, 1, dim>>, // connection allowed within a fixed comm range
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size of a node is read from this tag in the store
    color_tag<node_color, left_color, right_color> // colors of a node are read from these
);

}

}

#endif // FCPP_CASE_STUDY_SETUP_H_
