// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file graphic.cpp
 * @brief Runs a single execution of the message dispatch case study with a graphical user interface.
 */

#include "lib/process_management.hpp"
#include "lib/simulation_setup.hpp"

using namespace fcpp;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    std::cout << "/*\n";
    for (int dens = 10; dens <= 20; dens += 10)
        for (int hops = 5; hops <= 10; hops += 5)
            for (int speed = 0; speed <= 4; speed += 4) {
                // The network object type (interactive simulator with given options).
                using net_t = component::interactive_simulator<option::list>::net;
                // The initialisation values (simulation name, non-deterministic threshold, device speed, plotter object).
                auto init_v = common::make_tagged_tuple<option::name, option::epsilon, option::dens, option::hops, option::speed, option::plotter>(
                    "Dispatch of Peer-to-peer Messages (" + to_string(dens) + " dev/neigh, " + to_string(hops) + " hops, " + to_string(speed) + "% speed)",
                    0.1,
                    dens,
                    hops,
                    speed * 0.01,
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
