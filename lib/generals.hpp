// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file generals.hpp
 * @brief General utility functions, data types and tags used by the case studies.
 */

#ifndef FCPP_GENERALS_H_
#define FCPP_GENERALS_H_

#include "lib/beautify.hpp"
#include "lib/coordination.hpp"
#include "lib/data.hpp"


//! @brief Struct representing a message.
struct message {
    //! @brief Sender UID.
    fcpp::device_t from;
    //! @brief Receiver UID.
    fcpp::device_t to;
    //! @brief Creation timestamp.
    fcpp::times_t time;
    //! @brief Data content.
    fcpp::real_t data;

    //! @brief Empty constructor.
    message() = default;

    //! @brief Member constructor.
    message(fcpp::device_t from, fcpp::device_t to, fcpp::times_t time, fcpp::real_t data) : from(from), to(to), time(time), data(data) {}

    //! @brief Equality operator.
    bool operator==(message const& m) const {
        return from == m.from and to == m.to and time == m.time and data == m.data;
    }

    //! @brief Hash computation.
    size_t hash() const {
        constexpr size_t offs = sizeof(size_t)*CHAR_BIT/3;
        return (size_t(time) << (2*offs)) | (size_t(from) << (offs)) | size_t(to);
    }

    //! @brief Serialises the content from/to a given input/output stream.
    template <typename S>
    S& serialize(S& s) {
        return s & from & to & time & data;
    }

    //! @brief Serialises the content from/to a given input/output stream (const overload).
    template <typename S>
    S& serialize(S& s) const {
        return s << from << to << time << data;
    }
};


namespace std {
    //! @brief Hasher object for the message struct.
    template <>
    struct hash<message> {
        //! @brief Produces an hash for a message, combining to and from into a size_t.
        size_t operator()(message const& m) const {
            return m.hash();
        }
    };
}


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

namespace tags {
    //! @brief Legacy termination policy.
    struct legacy {};

    //! @brief Legacy termination policy with share.
    struct share {};

    //! @brief Novel termination policy.
    struct novel {};

    //! @brief Wave-like termination policy.
    struct wave {};


    //! @brief Spherical process.
    template <typename T>
    struct spherical {};

    //! @brief Tree process.
    template <typename T>
    struct tree {};


    //! @brief The maximum number of processes ever run by the node.
    template <typename T>
    struct max_proc {};

    //! @brief The total number of processes ever run by the node.
    template <typename T>
    struct tot_proc {};

    //! @brief Total time of first delivery.
    template <typename T>
    struct first_delivery_tot {};

    //! @brief Total number of first deliveries.
    template <typename T>
    struct delivery_count {};

    //! @brief Total number of repeated deliveries.
    template <typename T>
    struct repeat_count {};


    //! @brief Average time of first delivery.
    template <typename T>
    struct avg_delay {};

    //! @brief Total active processes per unit of time.
    template <typename T>
    struct avg_proc {};


    //! @brief The movement speed of devices.
    struct speed {};

    //! @brief The number of hops in the network.
    struct hops {};

    //! @brief The density of devices.
    struct dens {};

    //! @brief The number of devices.
    struct devices {};

    //! @brief The side of deployment area.
    struct side {};

    //! @brief Temporary data of active processes.
    struct proc_data {};

    //! @brief Total number of sent messages.
    struct sent_count {};

    //! @brief Color of the current node.
    struct node_color {};

    //! @brief Left color of the current node.
    struct left_color {};

    //! @brief Right color of the current node.
    struct right_color {};

    //! @brief Size of the current node.
    struct node_size {};

    //! @brief Shape of the current node.
    struct node_shape {};
} // tags


//! @brief Distance estimation which can only decrease over time.
FUN real_t monotonic_distance(ARGS, bool source) {
    return nbr(node, call_point, INF, [&](field<real_t> nd){
        real_t d = min_hood(CALL, nd + node.nbr_dist()); // inclusive
        return source ? 0 : d;
    });
}
//! @brief Export list for monotonic_distance.
FUN_EXPORT monotonic_distance_t = export_list<real_t>;


} // coordination


} // fcpp

#endif // FCPP_GENERALS_H_
