// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the message dispatch case study non-interactively from the command line, producing overall plots.
 */

#include "lib/process_management.hpp"
#include "lib/simulation_setup.hpp"

using namespace fcpp;

//! @brief Number of identical runs to be averaged.
constexpr int runs = 100;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = [&](std::string v){
        return batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(1, runs, 1), // 10 different random seeds
            batch::arithmetic<option::dens>(v == "dens" ? 10 : 20, 30, 20), // 3 different densities
            batch::arithmetic<option::hops>(v == "hops" ?  5 : 10, 15, 10), // 3 different hop sizes
            batch::arithmetic<option::speed>(v == "speed" ? 0.0 : 0.1, 0.2, v == "speed" ? 0.05 : 1), // 5 different speeds
            // generate output file name for the run
            batch::stringify<option::output>("output/batch", "txt"),
            batch::constant<option::plotter>(&p) // reference to the plotter object
        );
    };
    // Runs the given simulations.
    batch::run(comp_t{}, init_list("dens"), init_list("hops"), init_list("speed"));
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
