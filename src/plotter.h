#ifndef PLOTTER_H
#define PLOTTER_H

#include <Eigen/Core>
#include "tty.h"

template <int N> constexpr auto plot_points(double xscale, double yscale, const Eigen::Array<double, N, 2>& points) {
    using namespace tty;

    std::print("\n{}", cursor{ $forward = 32 });

    for (int i = 0; i < 23; ++i) { std::print("{}│\f", cursor{ $back = 1 }); }

    std::print("{}{:─^64}{}", cursor{ $up = 12, $back = 32 }, "┼", cursor{ $back = 65, $down = 12 });

    int j = 0;
    for (const auto& point : points.matrix().rowwise()) {
        int col = std::round((point(0) / xscale) * 32.0) + 31;
        int row = std::round((point(1) / yscale) * 12.0) + 11;
        std::print("{}{}{}",
                   cursor{ $forward = col, $up = row },
                   text{ j++, $fg = red },
                   cursor{ $back = col + 1, $down = row });
    }

}

#endif

