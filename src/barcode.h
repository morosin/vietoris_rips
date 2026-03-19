#ifndef BARCODE_H
#define BARCODE_H

#include "index_sequence_polyfill.h"
#include "matrix_formatters.h"
#include "simplicial_complex.h"
#include "tty.h"
#include <algorithm>
#include <format>
#include <map>
#include <print>
#include <set>

template <class Scale> struct simplex_lifetime {
    Scale start;
    Scale end;
};

template <simplicial_complex Cplx> class barcode {
    using scale_type = decltype(std::declval<Cplx>().epsilon());
    using lifetime = simplex_lifetime<scale_type>;
    // since we are aggregating simplices from different complexes these will have undeterminate edge
    // numbers, but the point indices are all that's used for comparison.
    std::multimap<simplex<>, lifetime> _lifetimes;
    scale_type _min_epsilon;
    scale_type _max_epsilon;

public:
    constexpr barcode(Cplx& cplx, scale_type stop_epsilon, scale_type step_epsilon)
        : _max_epsilon{ stop_epsilon } {
        std::set<simplex<>> active_simplices;
        this->_min_epsilon = cplx.epsilon();
        for (scale_type epsilon = cplx.epsilon(); epsilon < stop_epsilon + step_epsilon;
             epsilon += step_epsilon)
        {
            cplx.scale(epsilon);

            std::set<simplex<>> simplices{
                std::from_range, std::views::iota(1, cplx.n_points()) | std::views::transform([&](int n) {
                                     return &cplx.skeleton(n);
                                 }) | std::views::take_while([](chain_group<>* chains) {
                                     return chains->rank() > 0;
                                 }) | std::views::transform([](chain_group<>* chains) {
                                     return chains->generators();
                                 }) | std::views::join
            };

            std::vector<simplex<>> update_list;
            std::set_symmetric_difference(active_simplices.begin(),
                                          active_simplices.end(),
                                          simplices.begin(),
                                          simplices.end(),
                                          std::back_inserter(update_list));

            for (const auto& splx : update_list) {
                if (active_simplices.contains(splx)) {
                    auto& last = (std::ranges::subrange(_lifetimes.lower_bound(splx),
                                                        _lifetimes.upper_bound(splx))
                                  | std::views::reverse)
                                     .front();
                    last.second.end = epsilon;
                    active_simplices.erase(splx);
                } else {
                    active_simplices.insert(splx);
                    _lifetimes.insert(std::make_pair(splx, lifetime{ epsilon, scale_type{ -1 } }));
                }
            }
        }
    }

    constexpr scale_type min_epsilon() const { return _min_epsilon; }

    constexpr scale_type max_epsilon() const { return _max_epsilon; }

    constexpr auto& lifetimes(this auto& self) { return self._lifetimes; }
};

template <simplicial_complex... Cplxs> class simultaneous_barcode {
    using scale_type = std::common_type_t<decltype(std::declval<Cplxs>().epsilon())...>;
    using lifetime = simplex_lifetime<scale_type>;

    std::tuple<barcode<Cplxs>...> _bc;

public:
    constexpr simultaneous_barcode(barcode<Cplxs>&&... bc): _bc{ std::move(bc)... } { }

    template <std::size_t I> constexpr auto& get(this auto& self) { return std::get<I>(self._bc); }

    constexpr scale_type min_epsilon() const {
        return std::apply(
            [](const Cplxs&... cplxs) {
                return std::min({ cplxs.min_epsilon()... });
            },
            _bc);
    }

    constexpr scale_type max_epsilon() const {
        return std::apply(
            [](const Cplxs&... cplxs) {
                return std::max({ cplxs.max_epsilon()... });
            },
            _bc);
    }
};

template <simplicial_complex... Cplxs> struct std::formatter<simultaneous_barcode<Cplxs...>> {
    static constexpr int screen_width = 110;
    static constexpr int label_column_width = 6;

    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    constexpr int epsilon_to_cols(const simultaneous_barcode<Cplxs...>& sb, auto epsilon) const {
        return epsilon * (screen_width - label_column_width);
    }

    constexpr auto row_barcode_start_label(std::format_context& ctx, const simplex<>& splx, int current_row,
                                           auto label) const {
        using namespace tty;
        ctx.advance_to(std::format_to(ctx.out(),
                                      "{}{}",
                                      text{ $fg = 6,
                                            cursor{ $back = 1 },
                                            "▕"sv,
                                            cursor{ $up = 1, $back = 5 },
                                            std::format("{:4.2f}", label),
                                            "▕"sv },
                                      cursor{ $down = 1 }));
    }

    constexpr auto row_barcode_end_label(std::format_context& ctx, const simplex<>& splx, int current_row,
                                         auto label) const {
        using namespace tty;
        ctx.advance_to(std::format_to(
            ctx.out(),
            "{}{}",
            text{ $fg = 5, "▏"sv, cursor{ $up = 1, $back = 1 }, "▏"sv, std::format("{:4.2f}", label) },
            cursor{ $down = 1 }));
    }

    constexpr auto table_row(std::format_context& ctx, const simultaneous_barcode<Cplxs...>& sb,
                             const simplex<>& splx) const {
        using namespace tty;

        const auto& pts = splx.points().matrix();
        ctx.advance_to(
            std::format_to(ctx.out(), "{}{} ", pts, cursor{ $up = static_cast<int>(pts.rows()) - 1 }));
        template for (constexpr int I : std::make_index_sequence<sizeof...(Cplxs)>{}) {
            const auto max_epsilon_col = label_column_width
                                         + epsilon_to_cols(sb, sb.template get<I>().max_epsilon());
            auto [rb, re] = sb.template get<I>().lifetimes().equal_range(splx);
            int color = static_cast<int>(I + 1);
            for (const auto& [_, lt] : std::ranges::subrange(rb, re)) {
                auto start_col = label_column_width + epsilon_to_cols(sb, lt.start);
                auto end_col = (lt.end >= 0) ? (label_column_width + epsilon_to_cols(sb, lt.end))
                                             : max_epsilon_col;
                auto bar_width = end_col - start_col;
                ctx.advance_to(std::format_to(ctx.out(), "{}", cursor{ $col = start_col }));
                row_barcode_start_label(ctx, splx, I, lt.start);
                int i = start_col;
                for (; i < end_col; ++i) {
                    ctx.advance_to(std::format_to(ctx.out(), "{}", text{ $fg = color, "━"sv }));
                }
                if (lt.end < 0) {
                    for (; i < screen_width; ++i) {
                        ctx.advance_to(std::format_to(ctx.out(), "{}", text{ $fg = color, "𜹍"sv }));
                    }
                } else {
                    row_barcode_end_label(ctx, splx, I, lt.end);
                }
            }
            ctx.advance_to(std::format_to(ctx.out(), "\n\n"));
        }
    }

    constexpr auto format(const simultaneous_barcode<Cplxs...>& sb, std::format_context& ctx) const {
        using namespace tty;

        std::set<simplex<>> all_simplices;
        template for (constexpr int I : std::make_index_sequence<sizeof...(Cplxs)>{}) {
            ctx.advance_to(std::format_to(ctx.out(),
                                          "{}\n\n",
                                          text{ $fg = static_cast<int>(I + 1),
                                                $underline(underline_kind::SINGLE),
                                                std::formatter<Cplxs...[I]>::complex_name() }));
            const auto& bc = sb.template get<I>();
            all_simplices.insert_range(bc.lifetimes() | std::views::keys);
        }

        for (const auto& splx : all_simplices) {
            table_row(ctx, sb, splx);
            ctx.advance_to(std::format_to(ctx.out(),
                                          "{}{}\n",
                                          cursor{ $up = 1 },
                                          text{ $fg = 243, std::format("{:─>{}}", '\n', screen_width) }));
        }
        return ctx.out();
    }
};

#endif

