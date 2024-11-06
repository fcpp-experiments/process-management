// Copyright © 2024 Giorgio Audrito. All Rights Reserved.

/**
 * @file repctl_batch.cpp
 * @brief Case study on the replication of Past-CTL temporal operators (batch execution).
 */

// [INTRODUCTION]

//! Importing the FCPP library.
#include "lib/replicated_pastctl.hpp"

//! @brief Number of identical runs to be averaged.
constexpr int runs = 10;


//! @brief The main function.
int main() {
    using namespace fcpp;

    // Construct the plotter object.
    option::plotter_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    //double infospeed = 0.8 * communication_range;
    auto init_list = batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(runs + 1, 40*runs, 1, 1, runs), // 40x random seeds for the default setting
            batch::arithmetic<option::tvar>( 0,   40,   1,       (int)option::var_def<option::tvar>), // 41 different temporal variances
            batch::arithmetic<option::dens>( 8.0, 18.0, 0.25, (double)option::var_def<option::dens>), // 41 different densities
            batch::arithmetic<option::hops>( 6.0, 16.0, 0.25, (double)option::var_def<option::hops>), // 41 different hop sizes
            batch::arithmetic<option::speed>(0.0, 20.0, 0.5,  (double)option::var_def<option::speed>),// 41 different speeds
            // computes area side from dens and hops
            batch::formula<option::side, size_t>([](auto const& x) {
                double d = common::get<option::dens>(x);
                double h = common::get<option::hops>(x);
                return h * (2*d)/(2*d+1) * communication_range / sqrt(2.0) + 0.5;
            }),
            // computes device number from dens and side
            batch::formula<option::devices, size_t>([](auto const& x) {
                double d = common::get<option::dens>(x);
                double s = common::get<option::side>(x);
                return d*s*s/(3.141592653589793*communication_range*communication_range) + 0.5;
            }),
            batch::formula<option::infospeed, double>([](auto const& x) {
                double d = common::get<option::dens>(x);
                double s = common::get<option::speed>(x);
                return (0.08*d - 0.7) * s * 0.01 + 0.075*d*d - 1.6*d + 11;
            }),
            batch::constant<option::output, option::plotter>(nullptr, &p) // reference to the plotter object
    );
    // Runs the given simulations.
    batch::run(comp_t{}, init_list);
    // Builds the resulting plots.
    std::cout << plot::file("repctl_batch", p.build());
    return 0;
}
