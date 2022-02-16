// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the message dispatch case study non-interactively from the command line, producing overall plots.
 */

#include "lib/simulation_setup.hpp"

using namespace fcpp;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
	//        batch::arithmetic<option::seed>(0, 9, 1), // 10 different random seeds
						       //        batch::arithmetic<option::speed>(size_t(0), option::comm/10, option::comm/50), // 6 different speeds
						       batch::arithmetic<option::speed>(size_t(2), size_t(6), size_t(2)), // 6 different speeds
        // generate output file name for the run
        batch::stringify<option::output>("output/batch", "txt"),
        batch::constant<option::plotter>(&p) // reference to the plotter object
    );
    // Runs the given simulations.
    batch::run(comp_t{}, init_list);
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
