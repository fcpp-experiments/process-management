// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file batch.cpp
 * @brief Runs multiple executions of the message dispatch case study non-interactively from the command line, producing overall plots.
 */

#include "lib/process_management.hpp"
#include "lib/simulation_setup.hpp"

using namespace fcpp;

//! @brief Number of identical runs to be averaged.
constexpr int runs = 200;

//! @brief Generates an arithmetic sequence of parameter values (or a default, if S corresponds).
template <typename T, typename S>
inline auto param_seq(double mn, double mx, double step, S) {
    constexpr bool is_same = std::is_same<T,S>::value;
    return batch::arithmetic<T>(is_same ? mn : option::var_def<T>, is_same ? mx : option::var_def<T>, step);
}

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = [&](auto v){
        return batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(1, std::is_same<decltype(v), int>::value ? 40*runs : runs, 1), // 40x random seeds for the default setting
            param_seq<option::tvar>(0, 40, 1, v), // 41 different temporal variances
            param_seq<option::dens>(8, 28, 0.5, v), // 41 different densities
            param_seq<option::hops>(4, 24, 0.5, v), // 41 different hop sizes
            param_seq<option::speed>(0, 40, 1, v), // 41 different speeds
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
            batch::formula<option::infospeed>([](auto const& x) -> size_t {
                double d = common::get<option::dens>(x);
                double s = common::get<option::speed>(x) * 0.01;
                return (0.08*d - 0.7) * s + 0.075*d*d - 1.6*d + 11;
            }),
            batch::constant<option::plotter>(&p) // reference to the plotter object
        );
    };
    // Runs the given simulations.
    batch::run(comp_t{}, init_list(0), init_list(option::tvar{}), init_list(option::dens{}), init_list(option::hops{}), init_list(option::speed{}));
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
