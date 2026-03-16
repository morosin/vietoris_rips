#include "vr_complex.h"
#include "matrix_formatters.h"
#include <cmath>
#include <chrono>
#include <print>
#include <random>
#include "plotter.h"

int main(int argc, char* argv[]) {
    using namespace tty;

    double epsilon = 0.66;

    //Eigen::Array<double, 10, 2> points{ { -0.78994, -0.61318 }, { 0.94537, -0.32599 }, { 0.83787, -0.54587 }, { 0.97754, -0.21075 }, { 0.23489, -0.97202 }, { -0.88413, 0.46725 }, { -0.95717, 0.28954 }, { 0.91452, 0.40455 }, { 0.99522, 0.09769 }, { -0.98716, 0.15975 } };

    Eigen::Array<double, 10, 2> points{ };
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
    
    auto start = std::chrono::steady_clock::now(); 
    cech_complex<RR2> cplx{ points, epsilon };
    cplx.skeleton(10);
    auto end = std::chrono::steady_clock::now();

    std::println("{}\n", cplx);
    std::println("(finished in {})", text{ $fg = blue, std::chrono::duration_cast<std::chrono::microseconds>(end - start) });
    
    start = std::chrono::steady_clock::now(); 
    vietoris_rips_complex<RR2> vr_cplx{ points, epsilon / 2.0 };
    vr_cplx.skeleton(10);
    end = std::chrono::steady_clock::now();
    
    std::println("{}\n", vr_cplx);
    std::println("(finished in {})", text{ $fg = blue, std::chrono::duration_cast<std::chrono::microseconds>(end - start) });

}

