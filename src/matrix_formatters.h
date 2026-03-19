#ifndef MATRIX_FORMATTERS_H
#define MATRIX_FORMATTERS_H

#include "Eigen/Core"
#include "plotter.h"
#include "simplicial_complex.h"
#include "tty.h"
#include <format>
#include <meta>
#include <ranges>

template <class Derived> concept eigen_formattable = std::derived_from<
    std::decay_t<Derived>, Eigen::MatrixBase<std::decay_t<Derived>>>;

template <eigen_formattable Derived> struct std::formatter<Derived> {
    int max_col_width = 5;

    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    constexpr int coeff_width(const Eigen::MatrixBase<Derived>& m) const {
        if constexpr (std::integral<typename Eigen::MatrixBase<Derived>::Scalar>) {
            return 1 + (std::floor((std::log10(1 + m.cwiseAbs().maxCoeff()))));
        } else {
            return max_col_width + 2;
        }
    }

    constexpr auto total_inner_width(const Eigen::MatrixBase<Derived>& m) const {
        return (coeff_width(m) + 2) * m.cols();
    }

    template <class T> constexpr auto print_number(T x, int width, std::format_context& ctx) const {
        using namespace tty;
        if constexpr (std::integral<T>) {
            std::format_to(ctx.out(), "{: {}d} ", x, width + 1);
        } else {
            std::format_to(ctx.out(), "{: {}.{}f} ", x, width - 1, width - 2);
        }
    }

    constexpr auto format(const Eigen::MatrixBase<Derived>& m, std::format_context& ctx) const {
        using namespace tty;

        if (m.rows() == 0 || m.cols() == 0) {
            ctx.advance_to(std::format_to(ctx.out(), "▕🮀 🮀▏"));
            return ctx.out();
        }

        const int w = coeff_width(m);
        const int total_width = total_inner_width(m);

        auto left_border = [&, i = 0]() mutable {
            if (i == 0) {
                ctx.advance_to(std::format_to(ctx.out(), "▕🭽"));
            } else if (i == m.rows() - 1) {
                ctx.advance_to(std::format_to(ctx.out(), "▕🭼"));
            } else {
                ctx.advance_to(std::format_to(ctx.out(), "▕▏"));
            }
            ++i;
        };

        auto right_border = [&, i = 0]() mutable {
            if (i == 0) {
                ctx.advance_to(std::format_to(ctx.out(), "🭾▏\f"));
                ctx.advance_to(std::format_to(ctx.out(), "{}", cursor{ $back = total_width + 4 }));
            } else if (i == m.rows() - 1) {
                ctx.advance_to(std::format_to(ctx.out(), "🭿▏"));
            } else {
                ctx.advance_to(std::format_to(ctx.out(), "▕▏\f"));
                ctx.advance_to(std::format_to(ctx.out(), "{}", cursor{ $back = total_width + 4 }));
            }
            ++i;
        };

        if (m.rows() > 1) {
            for (auto row : m.rowwise()) {
                left_border();
                for (auto x : row) { print_number(x, w, ctx); }
                right_border();
            }
        } else {
            ctx.advance_to(std::format_to(ctx.out(), "▕🮀"));
            for (auto x : m.row(0)) { print_number(x, w, ctx); }

            ctx.advance_to(std::format_to(ctx.out(), "🮀▏"));
        }
        return ctx.out();
    }
};

template <std::ranges::range R> requires(eigen_formattable<std::ranges::range_value_t<R>>)
struct std::formatter<R> {
    using matrix_type = std::ranges::range_value_t<R>;

    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    constexpr auto format(const R& range, std::format_context& ctx) const {
        using namespace tty;

        std::formatter<matrix_type> f;

        std::optional<int> prev_rows{};
        int total_space{};
        for (auto it = std::ranges::begin(range); it != std::ranges::end(range); ++it) {
            const auto& m = *it;
            if (prev_rows && *prev_rows > 1) {
                if (total_space >= 110) {
                    ctx.advance_to(std::format_to(ctx.out(), ",\f\f{}", cursor{ $back = total_space - 5 }));
                    prev_rows.reset();
                    total_space = 4;
                } else {
                    ctx.advance_to(std::format_to(ctx.out(), ",{}", cursor{ $up = *prev_rows - 1 }));
                }
            }
            ctx.advance_to(f.format(m, ctx));
            total_space += f.total_inner_width(m) + 5;
            prev_rows = m.rows();
        }
        return ctx.out();
    }
};

template <simplicial_complex Cplx> struct std::formatter<Cplx> {
    static constexpr auto max_complex_length = 24;

    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    static consteval std::string_view complex_name() {
        return std::define_static_string(std::meta::display_string_of(^^Cplx));
    }

    constexpr auto format_chains_and_boundary(Cplx& cplx, int degree, std::format_context& ctx) const {
        using namespace tty;

        chain_group<>& chains = cplx.skeleton(degree);
        if (degree > 0) {
            auto bdy = chains.boundary().toDense();
            int rows = bdy.rows();

            ctx.advance_to(std::format_to(ctx.out(), "{}🭯\f", cursor{ $forward = 2 }));
            for (int i = 0; i <= rows; ++i) {
                ctx.advance_to(std::format_to(ctx.out(), "{}│\f", cursor{ $back = 1 }));
            }
            ctx.advance_to(std::format_to(ctx.out(),
                                          "{}{}",
                                          cursor{ $up = rows + 1, $forward = 2 },
                                          text{ $fg = cyan, "∂_"sv, degree }));
            ctx.advance_to(std::format_to(ctx.out(),
                                          ": {} {}={}\n\n",
                                          bdy,
                                          text{ $fg = cyan, "h_"sv, degree },
                                          text{ $fg = green, betti_number(cplx, degree) }));
        } else {
            ctx.advance_to(
                std::format_to(ctx.out(), "  {}\f{}🭯\f", text{ $fg = green, "0"sv }, cursor{ $back = 1 }));
            ctx.advance_to(std::format_to(ctx.out(),
                                          "{}│  {}{}\f",
                                          cursor{ $back = 1 },
                                          text{ $fg = cyan, "∂_0"sv },
                                          cursor{ $back = 5 }));
            ctx.advance_to(std::format_to(ctx.out(),
                                          "{}│  {}={}\n",
                                          cursor{ $back = 1 },
                                          text{ $fg = cyan, "h_0"sv },
                                          text{ $fg = green, betti_number(cplx, 0) }));
        }

        if (chains.rank() > 0) {
            ctx.advance_to(std::format_to(
                ctx.out(), "ℤ^{}, {}-simplices", text{ +$bold, $fg = green, (int)chains.rank() }, degree));
            ctx.advance_to(std::format_to(
                ctx.out(), ": {}\n", chains.generators() | std::views::transform([](const auto& splx) {
                                         return splx.points().matrix();
                                     })));
            return true;
        } else {
            ctx.advance_to(std::format_to(ctx.out(), "  {}\n", text{ $fg = green, "0"sv }));
            return false;
        }
    }

    constexpr auto format(Cplx& cplx, std::format_context& ctx) const {
        using namespace tty;

        ctx.advance_to(std::format_to(ctx.out(),
                                      "{}\n",
                                      text{ $underline = underline_kind::SINGLE,
                                            text{ $fg = yellow, complex_name() },
                                            " (𝜀="sv,
                                            text{ $fg = cyan, cplx.epsilon() },
                                            ")"sv }));
        plot_points(1.2, 1.2, cplx.points());

        for (int n = 0; n < max_complex_length; ++n) {
            if (!format_chains_and_boundary(cplx, n, ctx)) { return ctx.out(); }
        }

        ctx.advance_to(std::format_to(ctx.out(), "...\n"));

        return ctx.out();
    }
};

#endif

