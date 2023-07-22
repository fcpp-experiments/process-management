// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file common_setup.hpp
 * @brief Common simulation setup.
 */

#ifndef FCPP_COMMON_SETUP_H_
#define FCPP_COMMON_SETUP_H_

#include "lib/fcpp.hpp"
#include "lib/generals.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @cond INTERNAL
namespace coordination {
    struct main;   // forward declaration of main function
    struct main_t; // forward declaration of main exports
}
//! @endcond


//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Dimensionality of the space.
constexpr size_t dim = 3;

//! @brief End of simulated time.
constexpr size_t end = 100;

//! @brief Standard deviation for distance estimations.
constexpr size_t dist_dev = 30;

//! @brief Multiplier of hops for timeout (in rounds).
constexpr double timeout_coeff = 1;

//! @brief Multiplier of hops for stabilization delay (in rounds).
constexpr double stabilize_coeff = 0.5;

//! @brief Number of service types.
const size_t max_svc_id = 100;

//! @brief Maximum "file" size in number of messages.
const size_t max_file_size = 10;

//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;


//! @brief Struct holding default values for simulation parameters.
template <typename T>
struct var_def_t;

//! @brief Default tvar for simulations.
template <>
struct var_def_t<tvar> {
    constexpr static size_t value = 10;
};

//! @brief Default dens for simulations.
template <>
struct var_def_t<dens> {
    constexpr static size_t value = 10;
};

//! @brief Default hops for simulations.
template <>
struct var_def_t<hops> {
#ifndef NOSPHERE
    constexpr static size_t value = 20;
#else
    constexpr static size_t value = 10;
#endif
};

//! @brief Default speed for simulations.
template <>
struct var_def_t<speed> {
#ifndef NOTREE
    constexpr static size_t value = 0;
#else
    constexpr static size_t value = 10;
#endif
};

//! @brief Default values for simulation parameters.
template <typename T>
constexpr size_t var_def = var_def_t<T>::value;


//! @brief Maximum admissible value for a seed.
constexpr size_t seed_max = std::min<uintmax_t>(std::numeric_limits<uint_fast32_t>::max(), std::numeric_limits<intmax_t>::max());

//! @brief Shorthand for a constant numeric distribution.
template <intmax_t num, intmax_t den = 1>
using n = distribution::constant_n<double, num, den>;

//! @brief Shorthand for a uniform numeric distribution.
template <intmax_t max, intmax_t min=0>
using nu = distribution::interval_n<double, min, max>;

//! @brief Shorthand for an constant input distribution.
template <typename T, typename R = double>
using i = distribution::constant_i<R, T>;

//! @brief The randomised sequence of rounds for every node (about one every second, with 10% variance).
using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,
    distribution::weibull<i<tavg>, functor::mul<i<tvar>, i<tavg>>>,
    distribution::constant_n<times_t, end + 5*period>
>;

//! @brief The distribution of initial node positions (random in a given rectangle).
using rectangle_d = distribution::rect<n<0>, n<0>, n<20>, i<side>, i<side>, n<20>>;

}

}

#endif // FCPP_COMMON_SETUP_H_
