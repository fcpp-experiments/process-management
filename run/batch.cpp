// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the message dispatch case study non-interactively from the command line, producing overall plots.
 */

#include "lib/process_management.hpp"
#include "lib/simulation_setup.hpp"

using namespace fcpp;

//! @brief Number of identical runs to be averaged.
constexpr int runs = 10;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = [&](std::string v){
        return batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(0, runs-1, 1), // 10 different random seeds
            batch::arithmetic<option::tvar>(v == "tvar" ?  0 : 0.1, 0.5, 0.5), // 3 different temporal variances
            batch::arithmetic<option::dens>(v == "dens" ? 10 :  20,  30,  20), // 3 different densities
            batch::arithmetic<option::hops>(v == "hops" ?  5 :  10,  15,  10), // 3 different hop sizes
            batch::arithmetic<option::speed>(v == "speed" ? 0.0 : 0.1, 0.2, v == "speed" ? 0.05 : 1), // 5 different speeds
            // generate output file name for the run
            batch::stringify<option::output>("output/batch", "txt"),
            // computes area side from dens and hops
            batch::formula<option::side>([](auto const& x) -> size_t {
                double d = common::get<option::dens>(x);
                double h = common::get<option::hops>(x);
                return h * (2*d)/(2*d+1) * comm / sqrt(2.0) + 0.5;
            }),
            // computes device number from dens and side
            batch::formula<option::devices>([](auto const& x) -> size_t {
                double d = common::get<option::dens>(x);
                double s = common::get<option::side>(x);
                return d*s*s/(3.141592653589793*comm*comm) + 0.5;
            }),
            batch::constant<option::plotter>(&p) // reference to the plotter object
        );
    };
    // Runs the given simulations.
    batch::run(comp_t{}, init_list("tvar"), init_list("dens"), init_list("hops"), init_list("speed"));
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
