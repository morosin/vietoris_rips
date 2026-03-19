#include "alpha_shape_complex.h"
#include "barcode.h"
#include "matrix_formatters.h"
#include "plotter.h"
#include "vr_complex.h"
#include <chrono>
#include <cmath>
#include <print>
#include <random>

int main(int argc, char* argv[]) {
    using namespace tty;

    double epsilon = 0.66;

    // Eigen::Array<double, 10, 2> points{ { -0.78994, -0.61318 }, { 0.94537, -0.32599 }, { 0.83787, -0.54587
    // }, { 0.97754, -0.21075 }, { 0.23489, -0.97202 }, { -0.88413, 0.46725 }, { -0.95717, 0.28954 }, {
    // 0.91452, 0.40455 }, { 0.99522, 0.09769 }, { -0.98716, 0.15975 } };

    Eigen::Array<double, 10, 2> points{};
    std::random_device rd;
    std::mt19937_64 mt{ rd() };
    std::uniform_real_distribution<> dist{ 0.0, 2 * M_PI };
    std::uniform_real_distribution<> noise{ -0.2, 0.2 };

    for (int i = 0; i < points.rows(); ++i) {
        double theta = dist(mt);
        points(i, 0) = noise(mt) + std::cos(theta);
        points(i, 1) = noise(mt) + std::sin(theta);
    }

    plot_points(1.2, 1.2, points);

    cech_complex<RR2> cplx{ points, 0.055 };
    vietoris_rips_complex<RR2> vr_cplx{ points, 0.055 };
    alpha_shape_complex<RR2> a_cplx{ points, 0.055 };

    std::println("{}\n",
                 simultaneous_barcode{ barcode{ cplx, 0.66, 0.055 },
                                       barcode{ vr_cplx, 0.33, 0.0275 },
                                       barcode{ a_cplx, 0.66, 0.055 } });

    std::println("{}", cplx);
    std::println("{}", vr_cplx);
    std::println("{}", a_cplx);
}

