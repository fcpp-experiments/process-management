// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file graphic.cpp
 * @brief Runs a single execution of the message dispatch case study with a graphical user interface.
 */

#include "lib/xc_processes.hpp"
#include "lib/xc_setup.hpp"

using namespace fcpp;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    std::cout << "/*\n";
    int tvar = option::var_def<option::tvar>;
    int hops = option::var_def<option::hops>;
    int dens = option::var_def<option::dens>;
    int speed = option::var_def<option::speed>;
    int side = hops * (2*dens)/(2*dens+1.0) * comm / sqrt(2.0) + 0.5;
    int devices = dens*side*side/(3.141592653589793*comm*comm) + 0.5;
    double infospeed = (0.08*dens - 0.7) * speed * 0.01 + 0.075*dens*dens - 1.6*dens + 11;
    {
        // The network object type (interactive simulator with given options).
        using net_t = component::interactive_simulator<option::list>::net;
        // The initialisation values (simulation name, non-deterministic threshold, device speed, plotter object).
        auto init_v = common::make_tagged_tuple<option::name, option::tvar, option::dens, option::hops, option::speed, option::side, option::devices, option::infospeed, option::seed, option::plotter>(
            "Dispatch of Peer-to-peer Messages (" + to_string(dens) + " dev/neigh, " + to_string(hops) + " hops, " + to_string(speed) + "% speed, " + to_string(tvar) + "% tvar)",
            tvar,
            dens,
            hops,
            speed,
            side,
            devices,
            infospeed,
            1,
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
