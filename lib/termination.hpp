/**
 * @file process_termination.hpp
 * @brief Process termination management.
 */

#ifndef FCPP_PROCESS_TERMINATION_H_
#define FCPP_PROCESS_TERMINATION_H_

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/option/distribution.hpp"

#include "lib/generals.hpp"
#include "lib/simulation_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Generating distribution for distance estimations.
std::weibull_distribution<real_t> dist_distr = distribution::make<std::weibull_distribution>(real_t(1), real_t(dist_dev*0.01));


//! @brief Adjusted nbr_dist value accounting for errors.
FUN field<real_t> adjusted_nbr_dist(ARGS) {
    return node.nbr_dist() * rand_hood(CALL, dist_distr) + node.storage(tags::speed{}) * comm / period * node.nbr_lag();
}

//! @brief Legacy termination logic (COORD19).
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::legacy>) {
     bool terminating = s == status::terminated_output;
     bool terminated = old(CALL, terminating, [&](bool ot){
        return any_hood(CALL, nbr(CALL, ot), ot) or terminating;
     });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);
    if (exiting) s = status::external;
    else if (terminating) s = status::internal_output;
}
//! @brief Legacy termination logic updated to use share (LMCS2020) instead of rep+nbr.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t, message const&, T<tags::share>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool exiting = all_hood(CALL, nbr(CALL, terminated), terminated);
    if (exiting) s = status::external;
    else if (terminating) s = status::internal_output;
}
//! @brief Novel termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::ispp>) {
    using namespace tags;

    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid;
    double ds = monotonic_distance(CALL, source, adjusted_nbr_dist(CALL));
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
    }
}
//! @brief Wave-like termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::wispp>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid and old(CALL, true, false);
    double ds = monotonic_distance(CALL, source, adjusted_nbr_dist(CALL));
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
    }
}
//! @brief Export list for termination_logic.
FUN_EXPORT termination_logic_t = export_list<bool, monotonic_distance_t>;

} // coordination

} // fcpp

#endif // // FCPP_PROCESS_TERMINATION_H_
