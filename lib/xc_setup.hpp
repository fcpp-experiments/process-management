// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file simulation_setup.hpp
 * @brief Simulation setup for the process management case study.
 */

#ifndef FCPP_XC_SETUP_H_
#define FCPP_XC_SETUP_H_

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


//! @brief Struct holding default values for simulation parameters.
template <typename T>
struct var_def_t;

//! @brief Default tvar for simulations.
template <>
struct var_def_t<tvar> {
    constexpr static size_t value = 10;
};

//! @brief Default dens for simulations.
template <>
struct var_def_t<dens> {
    constexpr static size_t value = 10;
};

//! @brief Default hops for simulations.
template <>
struct var_def_t<hops> {
#ifndef NOSPHERE
    constexpr static size_t value = 20;
#else
    constexpr static size_t value = 10;
#endif
};

//! @brief Default speed for simulations.
template <>
struct var_def_t<speed> {
#ifndef NOTREE
    constexpr static size_t value = 0;
#else
    constexpr static size_t value = 10;
#endif
};

//! @brief Default values for simulation parameters.
template <typename T>
constexpr size_t var_def = var_def_t<T>::value;


//! @brief Maximum admissible value for a seed.
constexpr size_t seed_max = std::min<uintmax_t>(std::numeric_limits<uint_fast32_t>::max(), std::numeric_limits<intmax_t>::max());

//! @brief Shorthand for a constant numeric distribution.
template <intmax_t num, intmax_t den = 1>
using n = distribution::constant_n<double, num, den>;

//! @brief Shorthand for an constant input distribution.
template <typename T, typename R = double>
using i = distribution::constant_i<R, T>;

//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,
    distribution::weibull<i<tavg>, functor::mul<i<tvar>, i<tavg>>>,
    distribution::constant_n<times_t, end + 5*period>
>;

//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect<n<0>, n<0>, n<20>, i<side>, i<side>, n<20>>;


//! @brief Aggregators for a given test.
template <template<class> class T, typename S>
using test_aggr_t = aggregators<
#ifdef ALLPLOTS
    max_proc<T<S>>,            aggregator::max<int>,
    repeat_count<T<S>>,        aggregator::sum<size_t>,
#endif
    tot_proc<T<S>>,            aggregator::sum<int>,
    first_delivery_tot<T<S>>,  aggregator::sum<times_t>,
    delivery_count<T<S>>,      aggregator::sum<size_t>
>;

//! @brief Storage for a given test.
template <template<class> class T, typename S>
using test_store_t = tuple_store<
#ifdef ALLPLOTS
    max_proc<T<S>>,            int,
    repeat_count<T<S>>,        size_t,
#endif
    tot_proc<T<S>>,            int,
    first_delivery_tot<T<S>>,  times_t,
    delivery_count<T<S>>,      size_t
>;

//! @brief Functors for a given test.
template <template<class> class T, typename S>
using test_func_t = log_functors<
    avg_delay<T<S>>,    functor::div<aggregator::sum<first_delivery_tot<T<S>>>, aggregator::sum<delivery_count<T<S>>>>,
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>>>, distribution::constant<i<devices>>>
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
    test_lines_t<T, A, spherical, xc>,
#endif
#ifndef NOTREE
    test_lines_t<T, A, tree, xc>,
#endif
    plot::none
>;

//! @brief Time-based plot.
template <typename S, typename... Ts>
using single_plot_t = plot::split<S, plot::join<Ts>...>;

//! @brief Overall row of plots.
template <typename S, size_t t0 = 0>
using row_plot_t = plot::join<
#ifdef ALLPLOTS
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<max_proc, aggregator::max<int>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, plot::value<aggregator::sum<sent_count>>>>,
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<repeat_count, aggregator::sum<size_t>>>>,
#endif
    plot::filter<plot::time, filter::above<t0>, single_plot_t<S, lines_t<delivery_count, aggregator::sum<size_t>>>>,
    single_plot_t<S, lines_t<avg_proc, noaggr>>,
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
    multi_filter_t<row_plot_t<tvar, 50>,   dens, hops, speed>,
    multi_filter_t<row_plot_t<dens, 50>,   tvar, hops, speed>,
    multi_filter_t<row_plot_t<hops, 50>,   tvar, dens, speed>,
    multi_filter_t<row_plot_t<speed, 50>,  tvar, dens, hops>,
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
        node_shape,                     shape
    >,
    // the basic tags and corresponding aggregators to be logged
#ifdef ALLPLOTS
    aggregators<
        sent_count,         aggregator::sum<size_t>
    >,
#endif
    // further options for each test
#ifndef NOSPHERE
    test_option_t<spherical, xc>,
#endif
#ifndef NOTREE
    test_option_t<tree, xc>,
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
        tavg,               distribution::weibull<n<period>, functor::mul<i<tvar>, n<period, 100>>>
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

#endif // FCPP_XC_SETUP_H_
