// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file simulation_setup.hpp
 * @brief Simulation setup for the process management case study.
 */

#ifndef FCPP_SIMULATION_SETUP_H_
#define FCPP_SIMULATION_SETUP_H_

#include "lib/fcpp.hpp"
#include "lib/generals.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @cond INTERNAL
namespace coordination {
    struct main;   // forward declaration of main function
    struct main_t; // forward declaration of main exports
}
//! @endcond


//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Dimensionality of the space.
constexpr size_t dim = 3;

//! @brief End of simulated time.
constexpr size_t end = 50;


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;


//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,
    distribution::weibull_n<times_t, 10, 1, 10>,
    distribution::constant_n<times_t, end+2>
>;


//! @brief Maximum admissible value for a seed.
constexpr size_t seed_max = std::min<uintmax_t>(std::numeric_limits<uint_fast32_t>::max(), std::numeric_limits<intmax_t>::max());

//! @brief Shorthand for a constant numeric distribution.
template <intmax_t num, intmax_t den = 1>
using n = distribution::constant_n<double, num, den>;

//! @brief Shorthand for an constant input distribution.
template <typename T>
using i = distribution::constant_i<double, T>;

//! @brief Functor calculating X/(X+1.0).
template <typename X>
using func_ratio = functor::div<X, functor::add<n<1>, X>>;

//! @brief Functor calculating area side from density and hops.
using func_side = functor::mul<
    func_ratio<functor::mul<i<dens>, n<2>>>,
    functor::mul<i<hops>, n<comm*10000, 14142>>
>;

//! @brief Functor calculating device number from density and hops.
using func_dev = functor::cast<functor::add<functor::div<
    functor::mul<functor::pow<func_side, n<2>>, i<dens>>,
    n<31416*comm*comm, 10000>
>, n<1,2>>, size_t>;

//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect<n<0>, n<0>, n<20>, func_side, func_side, n<20>>;


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
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>, false>>, distribution::constant<func_dev>>
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
#ifndef NOSPHERE
    test_lines_t<T, A, spherical, legacy, share, novel, wave>,
#endif
#ifndef NOTREE
    test_lines_t<T, A, tree,      legacy, share, novel, wave>,
#endif
    plot::none
>;

//! @brief Time-based plot.
template <typename... Ts>
using time_plot_t = plot::split<plot::time, plot::join<Ts>...>;

//! @brief Overall row of plots.
using row_plot_t = plot::join<
    time_plot_t<lines_t<max_proc, aggregator::max<int>>>,
    time_plot_t<lines_t<avg_proc, noaggr>>,
    time_plot_t<lines_t<avg_delay, noaggr>>,
    time_plot_t<plot::value<aggregator::sum<sent_count, false>>>,
    time_plot_t<lines_t<delivery_count, aggregator::sum<size_t>>>,
    time_plot_t<lines_t<repeat_count, aggregator::sum<size_t>>>
>;

//! @brief Overall plot document (one page for every variable).
using plot_t = plot::join<
    plot::filter<hops, filter::equal<10>, plot::filter<speed, filter::equal<1, 10>, plot::split<dens, row_plot_t>>>,
    plot::filter<dens, filter::equal<20>, plot::filter<speed, filter::equal<1, 10>, plot::split<hops, row_plot_t>>>,
    plot::filter<hops, filter::equal<10>, plot::filter<dens, filter::equal<20>, plot::split<speed, row_plot_t>>>
>;


//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<false>,     // no multithreading on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN in process_management.hpp)
    exports<coordination::main_t>, // export type list (types used in messages)
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<sequence::periodic_n<1, 0, 1, end>>, // the sequence generator for log events on the network
    spawn_schedule<sequence::multiple<func_dev, n<0>>>, // the sequence generator of node creation events on the network
    // the basic contents of the node storage
    tuple_store<
        speed,                          double,
        devices,                        size_t,
        side,                           double,
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
#ifndef NOSPHERE
    test_option_t<spherical, legacy, share, novel, wave>,
#endif
#ifndef NOTREE
    test_option_t<tree,      legacy, share, novel, wave>,
#endif
    // data initialisation
    init<
        x,                  rectangle_d,
        seed,               distribution::interval_n<uint_fast32_t, 0, seed_max>,
        speed,              functor::mul<i<speed>, n<comm, period>>,
        side,               func_side,
        devices,            func_dev
    >,
    // general parameters to use for plotting
    extra_info<
        dens,   int,
        hops,   int,
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
