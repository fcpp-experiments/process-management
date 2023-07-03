// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file simulation_setup.hpp
 * @brief Simulation setup for the process management case study.
 */

#ifndef FCPP_SIMULATION_SETUP_H_
#define FCPP_SIMULATION_SETUP_H_

#include "lib/fcpp.hpp"
#include "lib/generals.hpp"

#include "lib/common_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace for component options.
namespace option {

//! @brief Aggregators for a given test.
template <template<class> class T, typename S>
using test_aggr_t = aggregators<
    max_proc<T<S>>,            aggregator::max<int>,
    repeat_count<T<S>>,        aggregator::sum<size_t>,
    max_msg_size<T<S>>,        aggregator::max<size_t>,
    tot_msg_size<T<S>>,        aggregator::sum<size_t>,
    tot_proc<T<S>>,            aggregator::sum<int>,
    first_delivery_tot<T<S>>,  aggregator::sum<times_t>,
    delivery_count<T<S>>,      aggregator::sum<size_t>
>;

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

//! @brief Functors for a given test.
template <template<class> class T, typename S>
using test_func_t = log_functors<
    avg_delay<T<S>>,    functor::div<aggregator::sum<first_delivery_tot<T<S>>, true>, aggregator::sum<delivery_count<T<S>>, false>>,
    avg_size<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_msg_size<T<S>>, false>>, distribution::constant<i<devices>>>,
    avgtot_size<T<S>>,  functor::div<functor::div<aggregator::sum<tot_msg_size<T<S>>, false>, distribution::constant<i<devices>>>, n<end>>,
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>, false>>, distribution::constant<i<devices>>>,
    avgtot_proc<T<S>>,  functor::div<functor::div<aggregator::sum<tot_proc<T<S>>, false>, distribution::constant<i<devices>>>, n<end>>
>;

//! @brief Dummy aggregator for functor tags.
struct noaggr {
    template <typename A>
    using result_type = common::tagged_tuple_t<A, A>;
};

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN in process_management.hpp)
    exports<coordination::main_t>, // export type list (types used in messages)
    retain<metric::retain<2>>, // retain time for messages
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<sequence::periodic_n<1, 0, 1, end>>, // the sequence generator for log events on the network
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
        hops,                           size_t
    >,
    // the basic tags and corresponding aggregators to be logged
    aggregators<
        sent_count,         aggregator::sum<size_t>
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
        hops,               i<hops>
    >,
    // general parameters to use for plotting
    extra_info<
        tvar,   double,
        dens,   double,
        hops,   double,
        speed,  double
    >,
    //plot_type<plot_t>, // the plot description to be used
    dimension<dim>, // dimensionality of the space
    connector<connect::fixed<comm, 1, dim>>, // connection allowed within a fixed comm range
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size of a node is read from this tag in the store
    color_tag<node_color, left_color, right_color> // colors of a node are read from these
);

}

}

#endif // FCPP_SIMULATION_SETUP_H_
