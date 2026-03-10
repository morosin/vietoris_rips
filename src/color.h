#ifndef COLOR_H
#define COLOR_H

#include "Eigen/Core"
#include <Eigen/Dense>

namespace color_conversions {
inline constexpr Eigen::Matrix3d ciergb_to_ciexyz{ { 0.49000, 0.31000, 0.20000 },
                                                   { 0.17697, 0.81240, 0.01063 },
                                                   { 0.00000, 0.01000, 0.99000 } };
inline const auto ciexyz_to_ciergb = ciergb_to_ciexyz.inverse();

inline constexpr Eigen::Matrix3d ciexyz_to_linear_rgb{ { +3.2406255, -1.5372080, -0.4986286 },
                                                       { -0.9689307, +1.8757561, +0.0415175 },
                                                       { +0.0557101, -0.2040211, +1.0569959 } };
inline constexpr Eigen::Matrix3d linear_rgb_to_ciexyz{ { 0.4124, 0.3576, 0.1805 },
                                                       { 0.2126, 0.7152, 0.0722 },
                                                       { 0.0193, 0.1192, 0.9505 } };

inline constexpr Eigen::Matrix3d ciexyz_to_oklab_m1{ { +0.8189330101, +0.3618667424, -0.1288597137 },
                                                     { +0.0329845436, +0.9293118715, +0.0361456387 },
                                                     { +0.0482003018, +0.2643662691, +0.6338517070 } };
inline const auto oklab_to_ciexyz_m1 = ciexyz_to_oklab_m1.inverse();

inline constexpr Eigen::Matrix3d ciexyz_to_oklab_m2{ { +0.2104542553, +0.7936177850, -0.0040720468 },
                                                     { +1.9779984951, -2.4285922050, +0.4505937099 },
                                                     { +0.0259040371, +0.7827717662, -0.8086757660 } };
inline const auto oklab_to_ciexyz_m2 = ciexyz_to_oklab_m2.inverse();

inline constexpr Eigen::Matrix3d oklab_to_linear_rgb_m2{ { +1.0000000000, +0.3963377774, +0.2158037573 },
                                                         { +1.0000000000, -0.1055613458, -0.0638541728 },
                                                         { +1.0000000000, -0.0894841775, -1.2914855480 } };

inline constexpr Eigen::Matrix3d oklab_to_linear_rgb_m1{ { +4.0767416621, -3.3077115913, +0.2309699292 },
                                                         { -1.2684380046, +2.6097574011, -0.3413193965 },
                                                         { -0.0041960863, -0.7034186147, +1.7076147010 } };

}; // namespace color_conversions

struct ciexyz {
    double X{ 0 }, Y{ 0.0 }, Z{ 0.0 };

    constexpr ciexyz() = default;

    constexpr ciexyz(double _X, double _Y, double _Z): X{ _X }, Y{ _Y }, Z{ _Z } { }

    constexpr ciexyz(Eigen::Vector3d vec): X{ vec[0] }, Y{ vec[1] }, Z{ vec[2] } { }
};

template <class T> concept color_space = std::convertible_to<T, ciexyz> and std::constructible_from<T, ciexyz>
                                         and std::is_default_constructible_v<T>;

struct ciergb {
    double R{ 0.0 }, G{ 0.0 }, B{ 0.0 };

    constexpr ciergb() = default;

    constexpr ciergb(double _R, double _G, double _B): R{ _R }, G{ _G }, B{ _B } { }

    constexpr ciergb(Eigen::Vector3d vec): R{ vec[0] }, G{ vec[1] }, B{ vec[2] } { }

    constexpr ciergb(ciexyz ciexyz)
        : ciergb{ color_conversions::ciexyz_to_ciergb * Eigen::Vector3d{ ciexyz.X, ciexyz.Y, ciexyz.Z } } { }

    constexpr operator ciexyz() const {
        return ciexyz{ color_conversions::ciergb_to_ciexyz * Eigen::Vector3d{ R, G, B } };
    }
};

struct ciexyy {
    double x{ 0.0 }, y{ 0.0 }, Y{ 0.0 };

    constexpr ciexyy() = default;

    constexpr ciexyy(double _x, double _y, double _Y): x{ _x }, y{ _y }, Y{ _Y } { }

    constexpr ciexyy(ciexyz ciexyz)
        : x{ ciexyz.X / (ciexyz.X + ciexyz.Y + ciexyz.Z) }, y{ ciexyz.Y / (ciexyz.X + ciexyz.Y + ciexyz.Z) },
          Y{ ciexyz.Y } { }

    constexpr operator ciexyz() const { return ciexyz{ x * (Y / y), Y, (1.0 - x - y) * (Y / y) }; }
};

struct srgb {
    static constexpr auto from_linear = [](double v) {
        if (v <= 0.0031308) {
            return 12.92 * v;
        } else {
            return 1.055 * std::pow(v, 1.0 / 2.4) - 0.055;
        }
    };

    static constexpr auto to_linear = [](double v) {
        if (v <= 0.04045) {
            return v / 12.92;
        } else {
            return std::pow((v + 0.055) / 1.055, 2.4);
        }
    };

    double r{ 0.0 }, g{ 0.0 }, b{ 0.0 };

    constexpr srgb() = default;

    constexpr srgb(double _r, double _g, double _b): r{ _r }, g{ _g }, b{ _b } { }

    constexpr srgb(Eigen::Vector3d linear)
        : r{ from_linear(linear[0]) }, g{ from_linear(linear[1]) }, b{ from_linear(linear[2]) } { }

    constexpr srgb(ciexyz xyz)
        : srgb{ color_conversions::ciexyz_to_linear_rgb * Eigen::Vector3d{ xyz.X, xyz.Y, xyz.Z } } { }

    constexpr operator ciexyz() const {
        return ciexyz{ color_conversions::linear_rgb_to_ciexyz
                       * Eigen::Vector3d{ to_linear(r), to_linear(g), to_linear(b) } };
    }
};

struct oklab {
    double L{ 0.0 }, a{ 0.0 }, b{ 0.0 };

    constexpr oklab() = default;

    constexpr oklab(double _L, double _a, double _b): L{ _L }, a{ _a }, b{ _b } { }

    constexpr oklab(Eigen::Vector3d vec): oklab{ vec[0], vec[1], vec[2] } { }

    constexpr oklab(ciexyz xyz)
        : oklab{ color_conversions::ciexyz_to_oklab_m2
                 * ((color_conversions::ciexyz_to_oklab_m1 * Eigen::Vector3d{ xyz.X, xyz.Y, xyz.Z })
                        .cwisePow(1.0 / 3.0)) } { }

    constexpr operator ciexyz() const {
        auto lms1 = color_conversions::oklab_to_ciexyz_m2 * Eigen::Vector3d{ L, a, b };
        return ciexyz{ color_conversions::oklab_to_ciexyz_m1 * lms1.cwisePow(3.0) };
    }

    constexpr operator srgb() const {
        auto lms1 = color_conversions::oklab_to_linear_rgb_m2 * Eigen::Vector3d{ L, a, b };
        return srgb{ color_conversions::oklab_to_linear_rgb_m1 * (lms1.cwisePow(3.0)) };
    }
};

struct oklch {
    double L{ 0.0 }, C{ 0.0 }, h{ 0.0 };

    constexpr oklch() = default;
    constexpr oklch(double _L, double _C, double _h): L{ _L }, C{ _C }, h{ _h } { }
    constexpr oklch(Eigen::Vector3d vec): oklch{ vec[0], vec[1], vec[2] } { } 
    constexpr oklch(oklab lab): L{ lab.L }, C{ std::sqrt(lab.a * lab.a + lab.b * lab.b) }, h{ std::atan2(lab.b, lab.a) } { }
    constexpr oklch(ciexyz xyz): oklch{ oklab{ xyz } } { }

    constexpr operator oklab() const {
        return oklab{ L, C * std::cos(h), C * std::sin(h) };
    }
};

#endif

