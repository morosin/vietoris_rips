#include "cech_complex.h"
#include "matrix_formatters.h"
#include <cmath>
#include <print>
#include <random>

int main(int argc, char* argv[]) {
    using namespace tty;

    Eigen::Array<double, 10, 2> points{};

    std::random_device rd;
    std::mt19937 mt{ rd() };
    std::uniform_real_distribution<> dist{ 0.0, 2 * M_PI };

    for (int i = 0; i < 10; ++i) {
        double theta = dist(mt);
        points(i, 0) = std::cos(theta);
        points(i, 1) = std::sin(theta);
    }

    cech_complex<RR2> cplx{ points, 0.66 };

    std::print("\n{}", cursor{ $forward = 32 });

    for (int i = 0; i < 23; ++i) { std::print("{}│\f", cursor{ $back = 1 }); }

    std::print("{}{:─^64}{}", cursor{ $up = 12, $back = 32 }, "┼", cursor{ $back = 65, $down = 12 });

    int j = 0;
    for (const auto& point : points.matrix().rowwise()) {
        int col = std::round(point(0) * 32.0) + 31;
        int row = std::round(point(1) * 12.0) + 11;
        std::print("{}{}{}",
                   cursor{ $forward = col, $up = row },
                   text{ j++, $fg = red },
                   cursor{ $back = col + 1, $down = row });
    }

    std::print("\n{}", cplx);
}

