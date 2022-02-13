// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file graphic.cpp
 * @brief Runs a single execution of the message dispatch case study with a graphical user interface.
 */

#include "lib/simulation_setup.hpp"

using namespace fcpp;

int main() {
    // Plot setup.
    option::plot_t p;
    std::cout << "/*\n";
    {
        // The network object type (interactive simulator with given options).
        using net_t = component::interactive_simulator<option::list>::net;
        // The initialisation values (simulation name, non-deterministic threshold, plotter object).
        auto init_v = common::make_tagged_tuple<option::name, option::epsilon, option::plotter>(
            "Dispatch of Peer-to-peer Messages",
            0.1,
            &p
        );
        // Construct the network object.
        net_t network{init_v};
        // Run the simulation until exit.
        network.run();
    }
    // Plot simulation results.
    std::cout << "*/\n";
    std::cout << plot::file("graphic", p.build());
    return 0;
}
