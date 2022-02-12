// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file process_common.hpp
 * @brief Aggregate process TODO.
 */

#ifndef FCPP_PROCESS_COMMON_H_
#define FCPP_PROCESS_COMMON_H_

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

    //! @brief Empty constructor.
    message() = default;

    //! @brief Member constructor.
    message(fcpp::device_t from, fcpp::device_t to, fcpp::times_t time) : from(from), to(to), time(time) {}

    //! @brief Equality operator.
    bool operator==(message const& m) const {
        return from == m.from and to == m.to and time == m.time;
    }

    //! @brief Hash computation.
    size_t hash() const {
        constexpr size_t offs = sizeof(size_t)*CHAR_BIT/3;
        return (size_t(time) << (2*offs)) | (size_t(from) << (offs)) | size_t(to);
    }

    //! @brief Serialises the content from/to a given input/output stream.
    template <typename S>
    S& serialize(S& s) {
        return s & from & to & time;
    }

    //! @brief Serialises the content from/to a given input/output stream (const overload).
    template <typename S>
    S& serialize(S& s) const {
        return s << from << to << time;
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


//! @brief Minimum number whose square is at least n.
constexpr size_t discrete_sqrt(size_t n) {
    size_t lo = 0, hi = n, mid = 0;
    while (lo < hi) {
        mid = (lo + hi)/2;
        if (mid*mid < n) lo = mid+1;
        else hi = mid;
    }
    return lo;
}

//! @brief Number of devices.
constexpr size_t devices = 300;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Side of the deployment area.
constexpr size_t side = discrete_sqrt(devices * 3000);

//! @brief Height of the deployment area.
constexpr size_t height = 100;

//! @brief Color hue scale.
constexpr float hue_scale = 360.0f/(side+height);


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {


namespace tags {
    //! @brief The movement speed of devices.
    struct speed {};

    //! @brief The maximum number of processes ever run by the node.
    template <typename T>
    struct max_proc {};

    //! @brief The total number of processes ever run by the node.
    template <typename T>    
    struct tot_proc {};

    //! @brief Total time of first delivery.
    template <typename T>    
    struct first_delivery {};

    //! @brief Total number of sent messages.
    template <typename T>    
    struct sent_count {};

    //! @brief Total number of first deliveries.
    template <typename T>    
    struct delivery_count {};

    //! @brief Total number of repeated deliveries.
    template <typename T>    
    struct repeat_count {};

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
}

template <typename node_t, typename G, typename S, typename... Ts, typename K = typename std::decay_t<S>::value_type, typename T = std::decay_t<std::result_of_t<G(K const&, Ts const&...)>>, typename R = std::decay_t<tuple_element_t<0,T>>, typename B = std::decay_t<tuple_element_t<1,T>>>
std::enable_if_t<std::is_same<B,status>::value, std::unordered_map<K, R>>
spawn_legacy(node_t& node, trace_t call_point, G&& process, S&& key_set, Ts const&... xs) {
    return spawn(CALL, [&](K const& key, Ts const&... xs) {
			   auto r = process(key, xs...);
			   return r;
			       } , key_set, xs...);
}

} // coordination

} // fcpp

#endif // FCPP_PROCESS_COMMON_H_
