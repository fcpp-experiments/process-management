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
    int tvar = 10;
    int dens = 20;
    for (int hops = 5; hops <= 10; hops += 5)
        for (int speed = 0; speed <= 10; speed += 10) {
            int side = hops * (2*dens)/(2*dens+1.0) * comm / sqrt(2.0) + 0.5;
            int devices = dens*side*side/(3.141592653589793*comm*comm) + 0.5;
            // The network object type (interactive simulator with given options).
            using net_t = component::interactive_simulator<option::list>::net;
            // The initialisation values (simulation name, non-deterministic threshold, device speed, plotter object).
            auto init_v = common::make_tagged_tuple<option::name, option::tvar, option::dens, option::hops, option::speed, option::side, option::devices, option::plotter>(
                "Dispatch of Peer-to-peer Messages (" + to_string(dens) + " dev/neigh, " + to_string(hops) + " hops, " + to_string(speed) + "% speed, " + to_string(tvar) + "% tvar)",
                tvar * 0.01,
                dens,
                hops,
                speed * 0.01,
                side,
                devices,
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
