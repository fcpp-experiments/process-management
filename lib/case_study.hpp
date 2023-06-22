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
#include "lib/termination.hpp"
#include "lib/simulation_setup.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp
{

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination
{

//! @brief Status of devices
enum class devstatus
{
    IDLE,   // nothing interesting
    DISCO,  // discovery of service
    OFFER,  // offer of service
    SERVED, // being served
    SERVING // serving
};

color status_color(const devstatus st, const size_t nproc)
{
    color sc;

    switch (st) {
    case devstatus::IDLE:
        sc = (nproc ? color(GREEN) : color(WHITE));
        break;
    case devstatus::DISCO:
        sc = color(BLUE);
        break;
    case devstatus::OFFER:
        sc = color(RED);
        break;
    case devstatus::SERVED:
        sc = color(SALMON);
        break;
    default:
        sc = color(BLACK);
    }

    return sc;
}

//! @brief Possibly generates a discovery message, given the number of devices.
FUN common::option<message> get_disco_message(ARGS, size_t devices) {
    common::option<message> m;

    // ***TODO*** --> set time to 10 
    // message just past time 10 from highest-id device
    if (node.uid == devices-1 && node.current_time() > 10 && node.storage(tags::sent_count{}) == 0) {
        // generate a discovery message for a random service type
        m.emplace(node.uid, 1, node.current_time(), 0.0, msgtype::DISCO, node.next_int(node.storage(tags::num_svc_types{}) - 1));
        node.storage(tags::sent_count{}) += 1;
    }
    return m;
}

//! @brief Simulates sending a file as a sequence of messages to another device.
FUN common::option<message> send_file_seq(ARGS, fcpp::device_t to, int sz=1) { CODE
    common::option<message> m;

    int cnt = counter(CALL) + 1;

    if (cnt <= sz) {
        m.emplace(node.uid, to, node.current_time(), 0.0, 
                  cnt < sz ? msgtype::DATA : msgtype::DATAEND, 
                  node.next_int());
    }

    return m;
}

//! @brief Process that does a spherical broadcast of a message.
GEN(T) message_log_type spherical_message(ARGS, common::option<message> const& m, T, bool render = false) { CODE
    message_log_type r = spawn_profiler(CALL, tags::spherical<T>{}, [&](message const& m){
        status s = node.uid == m.to ? status::terminated_output : status::internal;
        return make_tuple(node.current_time(), s);
    }, m, node.storage(tags::infospeed{}), render);

    return r;
}
FUN_EXPORT spherical_message_t = export_list<spawn_profiler_t>;

//! @brief Process that does a spherical broadcast of a service request.
GEN(T) message_log_type spherical_discovery(ARGS, common::option<message> const& m, T, bool render = false) { CODE
    message_log_type r = spawn_profiler(CALL, tags::spherical<T>{}, [&](message const &m) {
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

//! @brief Sends a message over a tree topology.
GEN(T) message_log_type tree_message(ARGS, common::option<message> const& m, T, device_t parent, set_t const &below, bool render = false) { CODE
    message_log_type r = spawn_profiler(CALL, tags::tree<T>{}, [&](message const &m) {
            bool source_path = any_hood(CALL, nbr(CALL, parent) == node.uid) or node.uid == m.from;
            bool dest_path = below.count(m.to) > 0;
            status s = node.uid == m.to ? status::terminated_output :
                    source_path or dest_path ? status::internal : status::external;

            // if I requested the offered service, I reply by producing output
            if (node.uid == m.to) {
                s = status::internal_output;
            }

            return make_tuple(node.current_time(), s); 
        }, m, 0.9, render);

    return r;
}
//! @brief Exports for the main function.
FUN_EXPORT tree_message_t = export_list<spawn_profiler_t>;

FUN bool timeout(ARGS) { CODE
    int t = counter(CALL);

    bool to = t > node.storage(tags::hops{}) * timeout_coeff;

    return to;
}
FUN_EXPORT timeout_t = export_list<int>;

//! @brief Result type of spawn calls dispatching messages.
using parametric_status_t = std::pair<devstatus, message>;

//! @brief Manages behavior of devices with an automaton.
FUN void device_automaton(ARGS, parametric_status_t &parst) { CODE
    // import tags for convenience
    using namespace tags;

    message_log_type rd, rtm, rdt;
    devstatus st = parst.first;
    message par = parst.second;
    common::option<message> md = common::option<message>{};
    common::option<message> mtm = common::option<message>{};
    common::option<message> mdt = common::option<message>{};

    // spanning tree definition: aggregate computation of parent and below set
    device_t parent = flex_parent(CALL, false, comm);
    set_t below = parent_collection(CALL, parent, set_t{node.uid}, [](set_t x, set_t const &y)
                                    {
                                        x.insert(y.begin(), y.end());
                                        return x; 
                                    });

    switch (st) {
    case devstatus::IDLE:
        // random message with 1% probability during time [1..50]
        md = get_disco_message(CALL, node.storage(tags::devices{}));
        break;
    case devstatus::DISCO:
        break;
    case devstatus::OFFER:
        if (parst.second.type == msgtype::DISCO) { // just transitioned
            parst.second.type == msgtype::OFFER;
            std::swap(parst.second.from, parst.second.to);
            mtm = parst.second;
        }
        break;
    // case devstatus::SERVED:
    //     if (parst.second.type == msgtype::OFFER) { // just transitioned
    //         parst.second.type == msgtype::ACCEPT;
    //         std::swap(parst.second.from, parst.second.to);
    //         mtm = parst.second;
    //     }
    //     break;
    // case devstatus::SERVING:
    //     if (parst.second.type == msgtype::ACCEPT) { // just transitioned
    //         mdt = send_file_seq(CALL, parst.second.from);
    //     }
    //     break;
    default:
        break;
    }

    rd = spherical_discovery(CALL, md, wispp{});
    // rtm = tree_message(CALL, mtm, ispp{}, parent, below);
    // #ifndef NOTREE
    // rdt = tree_message(CALL, mdt, ispp{}, parent, below);
    // #endif
    // #ifndef NOSPHERE
    // rdt = spherical_message(CALL, mdt, wispp{});
    // #endif

    switch (st) {
    case devstatus::IDLE:
        if (!md.empty()) { // transition to DISCO
            parst.first = devstatus::DISCO;
            parst.second = md;
        }

        if (rd.size()) { // transition to OFFER
            parst.first = devstatus::OFFER;
            // ASSUMPTION: if more than one candidate, OFFER only to first
            parst.second = (*rd.begin()).first;
        }
        break;
    case devstatus::DISCO:
        if (rtm.size()) { // transition to SERVED
            parst.first = devstatus::SERVED;
            // ASSUMPTION: if more than one candidate, get SERVED by first
            parst.second = (*rtm.begin()).first;
        } else if (timeout(CALL)) { // transition back to IDLE
            parst.first = devstatus::IDLE;                   
        }

        break;
    // case devstatus::OFFER:
    //     if (rtm.size()) { // transition to SERVING
    //         parst.first = devstatus::SERVING;
    //         // ASSUMPTION: if more than one candidate, SERVE the first
    //         parst.second = (*rtm.begin()).first;
    //     } else if (timeout(CALL)) { // transition back to IDLE
    //         parst.first = devstatus::IDLE;                   
    //     }
    //     break;
    // case devstatus::SERVING:
    //     if (mdt.size()) { // if all file sent, transition back to IDLE
    //         parst.first = devstatus::IDLE;
    //     }
    //     break;
    // case devstatus::SERVED:
    //     if (rdt.size() and parst.second.type==msgtype::DATAEND) { // if all file received, transition back to IDLE
    //         parst.first = devstatus::IDLE;
    //     }
    //     break;
    default:
        break;
    }
}
FUN_EXPORT device_automaton_t = common::export_list<spherical_discovery_t, spherical_message_t, flex_parent_t, real_t, parent_collection_t<set_t>, tree_message_t, timeout_t>;

//! @brief Main case study function.
MAIN() {
    // import tags for convenience
    using namespace tags;
    // random walk
    size_t l = node.storage(side{});
    rectangle_walk(CALL, make_vec(0, 0, 20), make_vec(l, l, 20), node.storage(speed{}) * comm / period, 1);

    old(CALL, parametric_status_t{devstatus::IDLE, message{}}, [&](parametric_status_t parst) {
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

        return parst; });
}
//! @brief Exports for the main function.
struct main_t : public export_list<rectangle_walk_t<3>, std::pair<devstatus, message>, device_automaton_t>
{
};

} // coordination

} // fcpp

#endif // FCPP_PROCESS_MANAGEMENT_H_
