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

    //! @brief Information Speed Process Propagation policy.
    struct ispp {};

    //! @brief Wave-like ISPP policy.
    struct wispp {};


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


    //! @brief The variance of round timing in the network.
    struct tvar {};

    //! @brief The number of hops in the network.
    struct hops {};

    //! @brief The density of devices.
    struct dens {};

    //! @brief The movement speed of devices.
    struct speed {};

    //! @brief The average round interval of a device.
    struct tavg {};

    //! @brief The number of devices.
    struct devices {};

    //! @brief The side of deployment area.
    struct side {};

    //! @brief The estimated multi-path information speed factor.
    struct infospeed {};

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


//! @brief Distance estimation which can only decrease over time using given metric field of relative distances.
GEN(T) real_t monotonic_distance(ARGS, bool source, field<T> const& rd) { CODE
    return nbr(CALL, INF, [&](field<real_t> nd){
        real_t mind = min_hood(CALL, nd + rd); // inclusive
        return source ? 0.0 : mind;
    });
}
//! @brief Export list for monotonic_distance.
FUN_EXPORT monotonic_distance_t = export_list<real_t>;


//! @brief Computes stable parents through FLEX distance estimation.
FUN device_t flex_parent(ARGS, bool source, real_t radius) { CODE
    constexpr real_t epsilon = 0.5;
    constexpr real_t distortion = 0.1;
    tuple<real_t, device_t> loc{source ? 0 : INF, node.uid};
    return get<1>(nbr(CALL, loc, [&] (field<tuple<real_t, device_t>> x) {
        field<real_t> dist = max(node.nbr_dist(), distortion*radius);
        tuple<real_t, device_t> const& old_di = self(CALL, x);
        real_t old_d = get<0>(old_di);
        device_t old_i = get<1>(old_di);
        field<real_t> nd = get<0>(x);
        tuple<real_t, device_t> new_di = min_hood(CALL, make_tuple(nd + dist, node.nbr_uid()), loc);
        real_t new_d = get<0>(new_di);
        device_t new_i = get<1>(new_di);
        tuple<real_t,real_t,real_t> slopeinfo = max_hood(CALL, make_tuple((old_d - nd)/dist, nd, dist), make_tuple(-INF, INF, 0));
        if (old_d == new_d or new_d == 0 or
            old_d > max(2*new_d, radius) or new_d > max(2*old_d, radius))
            return make_tuple(new_d, new_i);
        if (details::self(node.nbr_dist(), old_i) == INF or get<0>(details::self(x, old_i)) > old_d)
            old_i = new_i;
        if (get<0>(slopeinfo) > 1 + epsilon)
            return make_tuple(get<1>(slopeinfo) + get<2>(slopeinfo) * (1 + epsilon), new_i);
        if (get<0>(slopeinfo) < 1 - epsilon)
            return make_tuple(get<1>(slopeinfo) + get<2>(slopeinfo) * (1 - epsilon), new_i);
        return make_tuple(old_d, new_i);
    }));
}
//! @brief Export list for flex_parent.
FUN_EXPORT flex_parent_t = export_list<tuple<real_t, device_t>>;


//! @brief Collects distributed data with a single-path strategy according to given parents.
GEN(T,G,BOUND(G, T(T,T)))
T parent_collection(ARGS, device_t parent, T const& value, G&& accumulate) { CODE
    return nbr(CALL, T{}, [&](field<T> x){
        return fold_hood(CALL, accumulate, mux(nbr(CALL, parent) == node.uid, x, T{}), value);
    });
}
//! @brief Export list for parent_collection.
GEN_EXPORT(T) parent_collection_t = export_list<T, device_t>;


} // coordination


} // fcpp

#endif // FCPP_GENERALS_H_
