// Copyright © 2023 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the message dispatch case study non-interactively from the command line, producing overall plots.
 */

#include "lib/process_management.hpp"
#include "lib/simulation_setup.hpp"

using namespace fcpp;

//! @brief Number of identical runs to be averaged.
constexpr int runs = 1000;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(runs + 1, 40*runs, 1, 1, runs), // 40x random seeds for the default setting
            batch::arithmetic<option::tvar>(0,   40,   1,      (int)option::var_def<option::tvar>), // 41 different temporal variances
            batch::arithmetic<option::dens>(8.0, 28.0, 0.5, (double)option::var_def<option::dens>), // 41 different densities
            batch::arithmetic<option::hops>(4.0, 24.0, 0.5, (double)option::var_def<option::hops>), // 41 different hop sizes
            batch::arithmetic<option::speed>(0,  40,   1,      (int)option::var_def<option::speed>),// 41 different speeds
            // generate output file name for the run
            batch::stringify<option::output>("output/batch", "txt"),
            // computes area side from dens and hops
            batch::formula<option::side, size_t>([](auto const& x) {
                double d = common::get<option::dens>(x);
                double h = common::get<option::hops>(x);
                return h * (2*d)/(2*d+1) * comm / sqrt(2.0) + 0.5;
            }),
            // computes device number from dens and side
            batch::formula<option::devices, size_t>([](auto const& x) {
                double d = common::get<option::dens>(x);
                double s = common::get<option::side>(x);
                return d*s*s/(3.141592653589793*comm*comm) + 0.5;
            }),
            batch::constant<option::plotter>(&p) // reference to the plotter object
    );
    // Runs the given simulations.
    batch::run(comp_t{}, init_list);
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
