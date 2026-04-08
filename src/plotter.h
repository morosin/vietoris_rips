#ifndef PLOTTER_H
#define PLOTTER_H

#include "tty.h"
#include <Eigen/Core>
#include <print>

template <int N>
constexpr void plot_points(double xscale, double yscale, const Eigen::Array<double, N, 2>& points) {
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
    std::print("\n{}{}{}",
               cursor{ $col = 66, $up = static_cast<int>(points.rows()) },
               points.matrix(),
               cursor{ $up = static_cast<int>(points.rows()) - 1 });
    for (int i = 0; i < points.rows(); ++i) {
        std::print("{}{}", text{ $fg = 243, " "sv, i }, cursor{ $back = 2, $down = 1 });
    }
    std::print("\n");
}

#endif

