// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include "lib/fcpp.hpp"
#include "lib/spherical_process.hpp"

using namespace fcpp;
using namespace component::tags;
using namespace coordination::tags;


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
constexpr size_t devnum = 300;

//! @brief Communication radius.
constexpr size_t comm = 100;

//! @brief Side of the deployment area.
constexpr size_t width = discrete_sqrt(devnum * 3000);

constexpr size_t dim = 3;
constexpr size_t end = 1000;

//! @brief Average time of first delivery.
template <typename T>
struct avg_delay {};

//! @brief Total active processes per unit of time.
template <typename T>
struct avg_proc {};

using round_s = sequence::periodic<
    distribution::interval_n<times_t, 0, 1>,
    distribution::weibull_n<times_t, 10, 1, 10>,
    distribution::constant_n<times_t, end+2>
>;

using rectangle_d = distribution::rect_n<1, 0, 0, 20, width, width, 20>;

template <template<class> class T, typename S>
using test_aggr_t = aggregators<
    max_proc<T<S>>,            aggregator::max<int>,
    tot_proc<T<S>>,            aggregator::sum<int>,
    first_delivery_tot<T<S>>,  aggregator::sum<times_t>,
    delivery_count<T<S>>,      aggregator::sum<size_t>,
    repeat_count<T<S>>,        aggregator::sum<size_t>
>;
template <template<class> class T, typename S>
using test_store_t = tuple_store<
    max_proc<T<S>>,            int,
    tot_proc<T<S>>,            int,
    first_delivery_tot<T<S>>,  times_t,
    delivery_count<T<S>>,      size_t,
    repeat_count<T<S>>,        size_t
>;
template <template<class> class T, typename S>
using test_func_t = log_functors<
    avg_delay<T<S>>,    functor::div<aggregator::sum<first_delivery_tot<T<S>>, true>, aggregator::sum<delivery_count<T<S>>, false>>,
    avg_proc<T<S>>,     functor::div<functor::diff<aggregator::sum<tot_proc<T<S>>, false>>, distribution::constant_n<double, devnum>>
>;
template <template<class> class T, typename... Ss>
using test_option_t = common::type_sequence<test_aggr_t<T,Ss>..., test_store_t<T,Ss>..., test_func_t<T,Ss>...>;

struct noaggr {
    template <typename A>
    using result_type = common::tagged_tuple_t<A, A>;
};

template <template<class> class T, typename A, template<class> class P, typename... Ts>
using test_lines_t = plot::join<plot::value<typename A::template result_type<T<P<Ts>>>::tags::front>...>;

template <template<class> class T, typename A>
using lines_t = plot::join<test_lines_t<T, A, spherical, legacy>>;

template <typename... Ts>
using time_plot_t = plot::split<plot::time, plot::join<Ts>...>;

using plot_t = plot::join<
    time_plot_t<lines_t<max_proc, aggregator::max<int>>>,
    time_plot_t<lines_t<avg_proc, noaggr>>,
    time_plot_t<lines_t<avg_delay, noaggr>>,
    time_plot_t<plot::value<aggregator::sum<sent_count, false>>>,
    time_plot_t<lines_t<delivery_count, aggregator::sum<size_t>>>,
    time_plot_t<lines_t<repeat_count, aggregator::sum<size_t>>>
>;

DECLARE_OPTIONS(opt,
    parallel<true>,
    synchronised<false>,
    program<coordination::main>,
    exports<coordination::main_t>,
    round_schedule<round_s>,
    log_schedule<sequence::periodic_n<1, 0, 1, end>>,
    spawn_schedule<sequence::multiple_n<devnum, 0>>,
    tuple_store<
        speed,                          double,
        devices,                        size_t,
        side,                           size_t,
        proc_data,                      std::vector<color>,
        sent_count,                     size_t,
        node_color,                     color,
        left_color,                     color,
        right_color,                    color,
        node_size,                      double,
        node_shape,                     shape
    >,
    aggregators<
        sent_count,         aggregator::sum<size_t>
    >,
    test_option_t<spherical, legacy, share, novel, wave>,
    test_option_t<tree,      legacy, share, novel, wave>,
    init<
        x,                  rectangle_d,
        speed,              distribution::constant_n<double, 5>,
        devices,            distribution::constant_n<size_t, devnum>,
        side,               distribution::constant_n<size_t, width>
    >,
    plot_type<plot_t>,
    dimension<dim>,
    connector<connect::fixed<comm, 1, dim>>,
    message_size<true>,
    shape_tag<node_shape>,
    size_tag<node_size>,
    color_tag<node_color, left_color, right_color>
);

int main() {
    plot_t p;
    std::cout << "/*\n";
    {
        using net_t = component::interactive_simulator<opt>::net;
        auto init_v = common::make_tagged_tuple<name, epsilon, plotter>(
            "Dispatch of Peer-to-peer Messages",
            0.1,
            &p
        );
        net_t network{init_v};
        network.run();
    }
    std::cout << "*/\n";
    std::cout << plot::file("message_dispatch", p.build());
    return 0;
}
