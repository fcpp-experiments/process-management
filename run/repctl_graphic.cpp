// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file repctl_batch.cpp
 * @brief Case study on the replication of Past-CTL temporal operators (graphic execution).
 */

// [INTRODUCTION]

//! Importing the FCPP library.
#include "lib/replicated_pastctl.hpp"


//! @brief The main function.
int main() {
    using namespace fcpp;

    //! @brief The network object type (interactive simulator with given options).
    using net_t = component::interactive_simulator<option::list>::net;
    //! @brief Create the plotter object.
    option::plotter_t p;
    //! @brief The initialisation values (simulation name).
    int tvar = option::var_def<option::tvar>;
    int hops = option::var_def<option::hops>;
    int dens = option::var_def<option::dens>;
    int speed = option::var_def<option::speed>;
    int side = hops * (2*dens)/(2*dens+1.0) * communication_range / sqrt(2.0) + 0.5;
    int devices = dens*side*side/(3.141592653589793*communication_range*communication_range) + 0.5;
    double infospeed = (0.08*dens - 0.7) * speed * 0.01 + 0.075*dens*dens - 1.6*dens + 11;
    //0.8 * communication_range;
    auto init_v = common::make_tagged_tuple<option::name, option::tvar, option::dens, option::hops, option::speed, option::side, option::devices, option::infospeed, option::plotter>("Replicated Past-CTL", tvar, dens, hops, speed, side, devices, infospeed, &p);
    std::cout << "/*\n"; // avoid simulation output to interfere with plotting output
    {
        //! @brief Construct the network object.
        net_t network{init_v};
        //! @brief Run the simulation until exit.
        network.run();
    }
    std::cout << "*/\n"; // avoid simulation output to interfere with plotting output
    std::cout << plot::file("repctl_graphic", p.build()); // write plots
    return 0;
}
