// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file replicated_pastctl.cpp
 * @brief Case study on the replication of Past-CTL temporal operators.
 */

// [INTRODUCTION]

//! Importing the FCPP library.
#include "lib/fcpp.hpp"
#include "lib/replicated.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Number of people in the area.
constexpr size_t node_num = 150;
//! @brief Dimensionality of the space.
constexpr size_t dim = 2;
//! @brief The maximum communication range between nodes.
constexpr size_t communication_range = 100;
//! @brief The diagonal size.
constexpr size_t diag = 1000;
//! @brief The maximum x coordinate.
constexpr size_t hi_x = 800;
//! @brief The maximum y coordinate.
constexpr size_t hi_y = 600;
//! @brief End of simulated time.
constexpr size_t end = 100;

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Tags used in the node storage.
namespace tags {
    //! @brief Color of the current node.
    struct node_color {};
    //! @brief Size of the current node.
    struct node_size {};
    //! @brief Shape of the current node.
    struct node_shape {};

    //! @brief The variance of round timing in the network.
    struct tvar {};
    //! @brief The number of hops in the network.
    struct hops {};
    //! @brief The density of devices.
    struct dens {};
    //! @brief The movement speed of devices.
    struct speed {};
    //! @brief The number of devices.
    struct devices {};
    //! @brief The side of deployment area.
    struct side {};
    //! @brief The estimated multi-path information speed factor.
    struct infospeed {};
}

// [AGGREGATE PROGRAM]


//! @brief Main function.
MAIN() {
    using namespace tags;
    // call to the library function handling random movement
    rectangle_walk(CALL, make_vec(0,0), make_vec(node.storage(side{}), node.storage(side{})), node.storage(speed{})*communication_range, 1);
    // call to the case study function
    criticality_control(CALL, node.storage(hops{}), node.storage(infospeed{}));

    // display formula values in the user interface
    node.storage(node_size{}) = node.storage(critic{}) ? 20 : 10;
    node.storage(node_color{}) = color(node.storage(now_critic_replicated{}) ? RED : node.storage(now_critic_SLCS{}) ? YELLOW : GREEN);
    node.storage(node_shape{}) = node.storage(ever_critic{}) ? shape::cube : shape::sphere;
}
//! @brief Export types used by the main function (update it when expanding the program).
FUN_EXPORT main_t = export_list<rectangle_walk_t<2>, criticality_control_t>;

} // namespace coordination

// [SYSTEM SETUP]

//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

//! @brief Shorthand for a constant numeric distribution.
template <intmax_t num, intmax_t den = 1>
using n = distribution::constant_n<double, num, den>;

//! @brief Shorthand for an constant input distribution.
template <typename T, typename R = double>
using i = distribution::constant_i<R, T>;

//! @brief Description of the round schedule.
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,    // uniform time in the [0,1] interval for start
    distribution::weibull_n<times_t, 10, 1, 10>,// weibull-distributed time for interval (10/10=1 mean, 1/10=0.1 deviation)
    distribution::constant_n<times_t, end + 5>
>;
//! @brief The sequence of network snapshots (one every simulated second).
using log_s = sequence::periodic_n<1, 0, 1, end>;
//! @brief The sequence of node generation events (node_num devices all generated at time 0).
//spawn_schedule<sequence::multiple<i<devices, size_t>, n<0>>>
using spawn_s = sequence::multiple<i<devices, size_t>, n<0>>;
//! @brief The distribution of initial node positions (random in a rectangle).
using rectangle_d = distribution::rect<n<0>, n<0>, i<side>, i<side>>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    node_color,                 color,
    node_size,                  double,
    node_shape,                 shape,
    critic,                     bool,
    ever_critic,                bool,
	now_critic_SLCS,            bool,
    now_critic_replicated,      bool,
    error_SLCS,                 bool,
    error_replicated,           bool,
    seed,                       uint_fast32_t,
    speed,                      double,
    devices,                    size_t,
    side,                       size_t,
    infospeed,                  double,
    hops,                       hops_t
>;
//! @brief The tags and corresponding aggregators to be logged (change as needed).
using aggregator_t = aggregators<
    critic,                 aggregator::mean<double>,
    ever_critic,            aggregator::mean<double>,
    now_critic_SLCS,        aggregator::mean<double>,
    now_critic_replicated,  aggregator::mean<double>,
    error_SLCS,             aggregator::mean<double>,
    error_replicated,       aggregator::mean<double>
>;

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
    constexpr static size_t value = 10;
};

//! @brief Default speed for simulations.
template <>
struct var_def_t<speed> {
    constexpr static size_t value = 10;
};

//! @brief Default values for simulation parameters.
template <typename T>
constexpr size_t var_def = var_def_t<T>::value;

//! @brief Maximum admissible value for a seed.
constexpr size_t seed_max = std::min<uintmax_t>(std::numeric_limits<uint_fast32_t>::max(), std::numeric_limits<intmax_t>::max());

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

//! @brief Single generic plot description.
template <typename S>
using single_plot_t = plot::split<S, plot::values<aggregator_t, common::type_sequence<>, error_SLCS, error_replicated>>;

//! @brief Overall plot document (one plot for every variable).
using plotter_t = plot::join<
    multi_filter_t<single_plot_t<tvar>, speed, dens, hops>,
    multi_filter_t<single_plot_t<dens>, speed, tvar, hops>,
    multi_filter_t<single_plot_t<hops>, speed, tvar, dens>,
    multi_filter_t<single_plot_t<speed>, tvar, dens, hops>,
    multi_filter_t<plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, critic, ever_critic, now_critic_SLCS, now_critic_replicated>>, tvar, dens, hops, speed>
>;

//! @brief The general simulation options.
DECLARE_OPTIONS(list,
    parallel<true>,      // multithreading enabled on node rounds
    synchronised<false>, // optimise for asynchronous networks
    program<coordination::main>,   // program to be run (refers to MAIN above)
    exports<coordination::main_t>, // export type list (types used in messages)
    retain<metric::retain<3,1>>,   // messages are kept for 3 seconds before expiring
    round_schedule<round_s>, // the sequence generator for round events on nodes
    log_schedule<log_s>,     // the sequence generator for log events on the network
    spawn_schedule<spawn_s>, // the sequence generator of node creation events on the network
    store_t,       // the contents of the node storage
    aggregator_t,  // the tags and corresponding aggregators to be logged
    init<
        x,          rectangle_d, // initialise position randomly in a rectangle for new nodes
        seed,       functor::cast<distribution::interval_n<double, 0, seed_max>, uint_fast32_t>,
        infospeed,  i<infospeed>,
        speed,      functor::div<i<speed>, n<100>>,
        side,       i<side>,
        devices,    i<devices>,
        hops,       i<hops>
    >,
    // general parameters to use for plotting
    extra_info<
        tvar,   double,
        dens,   double,
        hops,   double,
        speed,  double
    >,
    plot_type<plotter_t>, // the plot description
    connector<connect::fixed<communication_range>>, // connection allowed within a fixed comm range
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size  of a node is read from this tag in the store
    color_tag<node_color>  // the color of a node is read from this tag in the store
);

} // namespace option

} // namespace fcpp
