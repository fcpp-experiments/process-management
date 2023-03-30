// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file case_study.hpp
 * @brief Case study on "service discovery and communication".
 */

#ifndef FCPP_CASE_STUDY_H_
#define FCPP_CASE_STUDY_H_

#include "lib/common/option.hpp"
#include "lib/component/calculus.hpp"
#include "lib/generals.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Status of devices
enum class devstatus {
    IDLE,    // nothing interesting
    DISCO,   // discovery of service
    OFFER,   // offer of service
    SERVED,  // being served
    SERVING  // serving
};

color status_color(const devstatus st, const size_t nproc) {
    color sc;

    switch (st) {
    case devstatus::IDLE:
        sc = (nproc ? color(GREEN) : color(WHITE));
        break;
    case devstatus::DISCO:
        sc = color(RED);
        break;
    case devstatus::OFFER:
        sc = color(BLUE);
        break;
    default:
        sc = color(BLACK);
    }

    return sc;
}

//! @brief Length of a round
constexpr size_t period = 1;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Possibly generates a discovery message, given the number of devices.
FUN common::option<message> get_disco_message(ARGS, size_t devices) {
    common::option<message> m;
    // random message with 1% probability during time [1..50]
    if (node.uid == devices-1 && node.current_time() > 1 && node.storage(tags::sent_count{}) == 0) {
        // generate a discovery message for a random service type
        m.emplace(node.uid, 0, node.current_time(), 0.0, msgtype::DISCO, node.next_int(node.storage(tags::num_svc_types{})-1));
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}


//! @brief Result type of spawn calls dispatching messages.
using message_log_type = std::unordered_map<message, times_t>;


//! @brief Computes stats on message delivery and active processes.
GEN(T) void proc_stats(ARGS, message_log_type const& nm, bool render, T) {
    // import tags for convenience
    using namespace tags;
    // stats on number of active processes
    int proc_num = node.storage(proc_data{}).size() - 1;
#ifdef ALLPLOTS
    node.storage(max_proc<T>{}) = max(node.storage(max_proc<T>{}), proc_num);
#endif
    node.storage(tot_proc<T>{}) += proc_num;
    // stats on delivery success
    old(node, call_point, message_log_type{}, [&](message_log_type m){
        for (auto const& x : nm) {
            if (m.count(x.first)) {
#ifdef ALLPLOTS
                node.storage(repeat_count<T>{}) += 1;
#endif
            } else {
                node.storage(first_delivery_tot<T>{}) += x.second - x.first.time;
                node.storage(delivery_count<T>{}) += 1;
                m[x.first] = x.second;
            }
        }
        return m;
    });
}
//! @brief Export list for proc_stats.
FUN_EXPORT proc_stats_t = export_list<message_log_type>;


//! @brief Novel termination logic.
template <typename node_t, template<class> class T>
void termination_logic(ARGS, status& s, real_t v, message const& m, T<tags::ispp>) {
    bool terminating = s == status::terminated_output;
    bool terminated = nbr(CALL, terminating, [&](field<bool> nt){
        return any_hood(CALL, nt) or terminating;
    });
    bool source = m.from == node.uid;
    double ds = monotonic_distance(CALL, source, node.nbr_dist());
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
    double ds = monotonic_distance(CALL, source, node.nbr_dist());
    double dt = monotonic_distance(CALL, source, node.nbr_lag());
    bool slow = ds < v * comm / period * (dt - period);
    if (terminated or slow) {
        if (s == status::terminated_output) s = status::border_output;
        if (s == status::internal) s = status::border;
    }
}
//! @brief Export list for termination_logic.
FUN_EXPORT termination_logic_t = export_list<bool, monotonic_distance_t>;


//! @brief Wrapper calling a spawn function with a given process and key set, while tracking the processes executed.
GEN(T,G,S) message_log_type spawn_profiler(ARGS, T, G&& process, S&& key_set, real_t v, bool render) {
    // dispatches messages
    message_log_type r = spawn(node, call_point, [&](message const& m){
        auto r = process(m);
        termination_logic(CALL, get<1>(r), v, m, T{});
        real_t key = get<1>(r) == status::external ? 0.5 : 1;
        node.storage(tags::proc_data{}).push_back(color::hsva(m.data * 360, key, key));
        return r;
    }, std::forward<S>(key_set));
    // compute stats
    proc_stats(CALL, r, render, T{});

    return r;
}
//! @brief Export list for spawn_profiler.
FUN_EXPORT spawn_profiler_t = export_list<spawn_t<message, status>, termination_logic_t, proc_stats_t>;


//! @brief Process that does a spherical broadcast of a service request.
FUN message_log_type spherical_discovery(ARGS, common::option<message> const& m, bool render = false) { CODE
    message_log_type r = spawn_profiler(CALL, tags::spherical<tags::wispp>{}, [&](message const& m){
        status s = status::internal;

        // if I offer a service matching the request, I reply by producing output
        if (m.svc_type == node.storage(tags::offered_svc{})) s = status::internal_output;

        return make_tuple(node.current_time(), s);
    }, m, node.storage(tags::infospeed{}), render);

    return r;
}
FUN_EXPORT spherical_discovery_t = export_list<spawn_profiler_t>;


//! @brief The type for a set of devices.
using set_t = std::unordered_set<device_t>;

//! @brief Makes test for tree processes.
FUN void tree_service(ARGS, common::option<message> const& m, device_t parent, set_t const& below, bool render = false) { CODE
    spawn_profiler(CALL, tags::tree<tags::ispp>{}, [&](message const& m){
        bool source_path = any_hood(CALL, nbr(CALL, parent) == node.uid) or node.uid == m.from;
        bool dest_path = below.count(m.to) > 0;
        status s = node.uid == m.to ? status::terminated_output :
                   source_path or dest_path ? status::internal : status::external;
        return make_tuple(node.current_time(), s);
    }, m, 0.9, render);
}
//! @brief Exports for the main function.
FUN_EXPORT tree_service_t = export_list<spawn_profiler_t>;


//! @brief Result type of spawn calls dispatching messages.
using parametric_status_t = std::pair<devstatus,message>;

//! @brief Manages behavior of devices with an automaton.
FUN void device_automaton(ARGS, parametric_status_t &parst) {
    message_log_type r;
    devstatus st = parst.first;
    message par = parst.second;

    switch (st) {
        case devstatus::IDLE:
        {
           	// random message with 1% probability during time [1..50]
            common::option<message> m = get_disco_message(CALL, node.storage(tags::devices{}));

            if (!m.empty()) { // transition to DISCO
                //node.storage(tags::offered_svc{}) = m.front().svc_type;
                parst.first = devstatus::DISCO;
                parst.second = m;
            }

    	    r = spherical_discovery(CALL, m, true);  

            // transitions
            if (r.size()) { // transition to OFFER
                parst.first = devstatus::OFFER;
                parst.second = (*r.begin()).first;
            }

            // spanning tree definition
            device_t parent = flex_parent(CALL, false, comm);
            // routing sets along the tree
            set_t below = parent_collection(CALL, parent, set_t{node.uid}, [](set_t x, set_t const &y) {
       									       x.insert(y.begin(), y.end());
       									       return x;});
            tree_service(CALL, common::option<message>{}, parent, below, false);

            break;
        }
        case devstatus::DISCO:
            r = spherical_discovery(CALL, common::option<message>{}, true);

            break;
        case devstatus::SERVED:
            break;
        case devstatus::OFFER:
            r = spherical_discovery(CALL, common::option<message>{}, true);

            break;
        case devstatus::SERVING:
            break;
        default:
            break;
    }
}
FUN_EXPORT device_automaton_t = common::export_list<spherical_discovery_t>;

//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0,0,20), make_vec(l,l,20), node.storage(speed{}) * comm / period, 1);

    old(CALL, parametric_status_t{devstatus::IDLE,message{}}, [&](parametric_status_t parst){
       	// basic node rendering
       	bool is_src = false;
       	bool highlight = is_src or node.uid == node.storage(devices{}) - 1;
       	node.storage(node_shape{}) = is_src ? shape::icosahedron : highlight ? shape::cube : shape::sphere;
       	node.storage(node_size{}) = highlight ? 20 : 10;
        // clear up stats data
        node.storage(proc_data{}).clear();
        node.storage(proc_data{}).push_back(color::hsva(0, 0, 0.3, 1));
        
        device_automaton(CALL, parst);       	

        devstatus st = parst.first;
        int proc_num = node.storage(proc_data{}).size() - 1;
        node.storage(node_color{}) = status_color(st, proc_num);
        if (proc_num > 0) node.storage(node_size{}) *= 1.5;

        return parst;
    });
}
//! @brief Exports for the main function.
    struct main_t : public export_list<rectangle_walk_t<3>, std::pair<devstatus,message>, spherical_discovery_t, flex_parent_t, real_t, parent_collection_t<set_t>, tree_service_t> {};


} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
