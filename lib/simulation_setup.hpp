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
    first_delivery_tot<T<S>>,  aggregator::only_finite<aggregator::sum<times_t>>,
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
    avg_delay<T<S>>,    functor::div<aggregator::only_finite<aggregator::sum<first_delivery_tot<T<S>>>>, aggregator::sum<delivery_count<T<S>>>>,
    avg_size<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_msg_size<T<S>>>>, i<devices>>,
    avgtot_size<T<S>>,  functor::div<functor::div<aggregator::sum<tot_msg_size<T<S>>>, i<devices>>, i<end_time>>,
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>>>, i<devices>>,
    avgtot_proc<T<S>>,  functor::div<functor::div<aggregator::sum<tot_proc<T<S>>>, i<devices>>, i<end_time>>
>;

//! @brief Overall options (aggregator, storage, functors) for given tests.
template <template<class> class T, typename... Ss>
using test_option_t = common::type_sequence<test_aggr_t<T,Ss>..., test_store_t<T,Ss>..., test_func_t<T,Ss>...>;


//! @brief Dummy aggregator for functor tags.
struct noaggr {
    template <typename A>
    using result_type = common::tagged_tuple_t<A, A>;
};

//! @brief Lines for a given data and test.
template <template<class> class T, typename A, template<class> class P, typename... Ts>
using test_lines_t = plot::join<plot::value<typename A::template result_type<T<P<Ts>>>::tags::front, aggregator::only_finite<aggregator::stats<double>>>...>;

//! @brief Lines for a given data and every test.
template <template<class> class T, typename A>
using lines_t = plot::join<
#ifndef NOSPHERE
    test_lines_t<T, A, spherical, legacy, share, ispp, wispp>,
#endif
#ifndef NOTREE
    test_lines_t<T, A, tree,      legacy, share, ispp, wispp>,
#endif
    plot::none
>;

//! @brief Time-based plot.
template <typename S, typename... Ts>
using single_plot_t = plot::split<S, plot::join<Ts>...>;

//! @brief Overall row of plots.
template <typename S, bool is_time = std::is_same<S,plot::time>::value, size_t t0 = is_time ? 0 : 50>
using row_plot_t = plot::join<
#ifdef ALLPLOTS
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<max_proc, aggregator::max<int>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, plot::value<aggregator::sum<sent_count>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<repeat_count, aggregator::sum<size_t>>>>,
#endif
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<delivery_count, aggregator::sum<size_t>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, std::conditional_t<is_time, lines_t<avg_proc, noaggr>, lines_t<avgtot_proc, noaggr>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, std::conditional_t<is_time, lines_t<avg_size, noaggr>, lines_t<avgtot_size, noaggr>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<max_msg_size, aggregator::max<size_t>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<avg_delay, noaggr>>>
>;

//! @brief Applies multiple filters (empty overload).
template <typename P, typename... Ts>
struct multi_filter {
    using type = P;
};

//! @brief Applies multiple filters (active overload).
template <typename P, typename T, typename... Ts>
struct multi_filter<P,T,Ts...> {
    using type = plot::filter<T, filter::equal<var_def<T>>, typename multi_filter<P,Ts...>::type>;
};

//! @brief Applies multiple filters (helper template).
template <typename P, typename... Ts>
using multi_filter_t = typename multi_filter<plot::split<common::type_sequence<Ts...>, P>, Ts...>::type;

//! @brief Overall plot document (one page for every variable).
using plot_t = plot::join<
#ifndef GRAPHICS
    multi_filter_t<row_plot_t<tvar>,  dens, hops, speed>,
    multi_filter_t<row_plot_t<dens>,  tvar, hops, speed>,
    multi_filter_t<row_plot_t<hops>,  tvar, dens, speed>,
    multi_filter_t<row_plot_t<speed>, tvar, dens, hops>,
#endif
    multi_filter_t<row_plot_t<plot::time>, tvar, dens, hops, speed>
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
        hops,                           size_t
    >,
    // the basic tags and corresponding aggregators to be logged
    aggregators<
        sent_count,         aggregator::sum<size_t>
    >,
    // further options for each test
#ifndef NOSPHERE
    test_option_t<spherical, legacy, share, ispp, wispp>,
#endif
#ifndef NOTREE
    test_option_t<tree,      legacy, share, ispp, wispp>,
#endif
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
        hops,               i<hops>,
        end_time,           i<end_time>
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

#endif // FCPP_SIMULATION_SETUP_H_
