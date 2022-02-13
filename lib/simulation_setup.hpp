// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file simulation_setup.hpp
 * @brief Simulation setup for the process management case study.
 */

#ifndef FCPP_SIMULATION_SETUP_H_
#define FCPP_SIMULATION_SETUP_H_

#include "lib/fcpp.hpp"
#include "process_management.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

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
constexpr size_t devnum = 300;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Side of the deployment area.
constexpr size_t width = discrete_sqrt(devnum * 3000);

//! @brief Dimensionality of the space.
constexpr size_t dim = 3;

//! @brief End of simulated time.
constexpr size_t end = 1000;

//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,
    distribution::weibull_n<times_t, 10, 1, 10>,
    distribution::constant_n<times_t, end+2>
>;

//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect_n<1, 0, 0, 20, width, width, 20>;

//! @brief Aggregators for a given test.
template <template<class> class T, typename S>
using test_aggr_t = aggregators<
    max_proc<T<S>>,            aggregator::max<int>,
    tot_proc<T<S>>,            aggregator::sum<int>,
    first_delivery_tot<T<S>>,  aggregator::sum<times_t>,
    delivery_count<T<S>>,      aggregator::sum<size_t>,
    repeat_count<T<S>>,        aggregator::sum<size_t>
>;

//! @brief Storage for a given test.
template <template<class> class T, typename S>
using test_store_t = tuple_store<
    max_proc<T<S>>,            int,
    tot_proc<T<S>>,            int,
    first_delivery_tot<T<S>>,  times_t,
    delivery_count<T<S>>,      size_t,
    repeat_count<T<S>>,        size_t
>;

//! @brief Functors for a given test.
template <template<class> class T, typename S>
using test_func_t = log_functors<
    avg_delay<T<S>>,    functor::div<aggregator::sum<first_delivery_tot<T<S>>, true>, aggregator::sum<delivery_count<T<S>>, false>>,
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>, false>>, distribution::constant_n<double, devnum>>
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
using test_lines_t = plot::join<plot::value<typename A::template result_type<T<P<Ts>>>::tags::front>...>;

//! @brief Lines for a given data and every test.
template <template<class> class T, typename A>
using lines_t = plot::join<
    test_lines_t<T, A, spherical, legacy, share, novel, wave>,
    test_lines_t<T, A, tree,      legacy, share, novel, wave>
>;

//! @brief Time-based plot.
template <typename... Ts>
using time_plot_t = plot::split<plot::time, plot::join<Ts>...>;

//! @brief Overall plot page.
using plot_t = plot::join<
    time_plot_t<lines_t<max_proc, aggregator::max<int>>>,
    time_plot_t<lines_t<avg_proc, noaggr>>,
    time_plot_t<lines_t<avg_delay, noaggr>>,
    time_plot_t<plot::value<aggregator::sum<sent_count, false>>>,
    time_plot_t<lines_t<delivery_count, aggregator::sum<size_t>>>,
    time_plot_t<lines_t<repeat_count, aggregator::sum<size_t>>>
>;

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<true>,      // multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<sequence::periodic_n<1, 0, 1, end>>, // the sequence generator for log events on the network
    spawn_schedule<sequence::multiple_n<devnum, 0>>, // the sequence generator of node creation events on the network
    // the basic contents of the node storage
    tuple_store<
        speed,                          double,
        devices,                        size_t,
        side,                           size_t,
        proc_data,                      std::vector<color>,
        sent_count,                     size_t,
        node_color,                     color,
        left_color,                     color,
        right_color,                    color,
        node_size,                      double,
        node_shape,                     shape
    >,
    // the basic tags and corresponding aggregators to be logged
    aggregators<
        sent_count,         aggregator::sum<size_t>
    >,
    // further options for each test
    test_option_t<spherical, legacy, share, novel, wave>,
    test_option_t<tree,      legacy, share, novel, wave>,
    // data initialisation
    init<
        x,                  rectangle_d,
        speed,              distribution::constant_n<double, 5>,
        devices,            distribution::constant_n<size_t, devnum>,
        side,               distribution::constant_n<size_t, width>
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
