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
constexpr size_t node_num = 100;
//! @brief Dimensionality of the space.
constexpr size_t dim = 2;
//! @brief The maximum communication range between nodes.
constexpr size_t communication_range = 100;
//! @brief The maximum x coordinate.
constexpr size_t hi_x = 800;
//! @brief The maximum y coordinate.
constexpr size_t hi_y = 600;

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
}

// [AGGREGATE PROGRAM]


//! @brief Main function.
MAIN() {
    using namespace tags;
    // call to the library function handling random movement
    rectangle_walk(CALL, make_vec(0,0), make_vec(hi_x, hi_y), 0.1*node.storage(comm_rad{}), node.storage(period{}));
    // call to the case study function
    criticality_control(CALL);

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

//! @brief Description of the round schedule.
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,    // uniform time in the [0,1] interval for start
    distribution::weibull_n<times_t, 10, 1, 10> // weibull-distributed time for interval (10/10=1 mean, 1/10=0.1 deviation)
>;
//! @brief The sequence of network snapshots (one every simulated second).
using log_s = sequence::periodic_n<1, 0, 1, 60>;
//! @brief The sequence of node generation events (node_num devices all generated at time 0).
using spawn_s = sequence::multiple_n<node_num, 0>;
//! @brief The distribution of initial node positions (random in a rectangle).
using rectangle_d = distribution::rect_n<1, 0, 0, hi_x, hi_y>;
//! @brief The contents of the node storage as tags and associated types.
using store_t = tuple_store<
    node_color,                 color,
    node_size,                  double,
    node_shape,                 shape,
    critic,                     bool,
    ever_critic,                bool,
	now_critic_SLCS,            bool,
    now_critic_replicated,      bool,
    diameter,                   hops_t,
    comm_rad,                   real_t,
    period,                     times_t
>;
//! @brief The tags and corresponding aggregators to be logged (change as needed).
using aggregator_t = aggregators<
    critic,                 aggregator::mean<double>,
    ever_critic,            aggregator::mean<double>,
    now_critic_SLCS,        aggregator::mean<double>,
    now_critic_replicated,  aggregator::mean<double>
>;

//! @brief Plot description.
using plotter_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, critic, ever_critic, now_critic_SLCS, now_critic_replicated>>;

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
        diameter,   distribution::constant_n<hops_t, (hi_x + hi_y)/communication_range>,
        comm_rad,   distribution::constant_n<real_t, communication_range>,
        period,     distribution::constant_n<times_t, 1>
    >,
    plot_type<plotter_t>, // the plot description
    area<0, 0, hi_x, hi_y>, // bounding coordinates of the simulated space
    connector<connect::fixed<communication_range>>, // connection allowed within a fixed comm range
    shape_tag<node_shape>, // the shape of a node is read from this tag in the store
    size_tag<node_size>,   // the size  of a node is read from this tag in the store
    color_tag<node_color>  // the color of a node is read from this tag in the store
);

} // namespace option

} // namespace fcpp


//! @brief The main function.
int main() {
    using namespace fcpp;

    //! @brief The network object type (interactive simulator with given options).
    using net_t = component::interactive_simulator<option::list>::net;
    //! @brief Create the plotter object.
    option::plotter_t p;
    //! @brief The initialisation values (simulation name).
    auto init_v = common::make_tagged_tuple<option::name, option::plotter>("Replicated Past-CTL", &p);
    std::cout << "/*\n"; // avoid simulation output to interfere with plotting output
    {
        //! @brief Construct the network object.
        net_t network{init_v};
        //! @brief Run the simulation until exit.
        network.run();
    }
    std::cout << "*/\n"; // avoid simulation output to interfere with plotting output
    std::cout << plot::file("replicated_pastctl", p.build()); // write plots
    return 0;
}
