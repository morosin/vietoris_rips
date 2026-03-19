#ifndef ISO_SUPPORT_TTY_H
#define ISO_SUPPORT_TTY_H

#include "aspect.h"
#include "element.h"
#include "static_string.h"
#include "color.h"
#include <algorithm>
#include <format>
#include <string_view>

namespace tty {
using namespace std::string_view_literals;

static constexpr auto ESC = "\033"sv;

enum color_mode { XTERM_256COLOR = 8, DIRECT = 24 };

template <color_mode Mode> struct color;

template <> struct color<XTERM_256COLOR> {
    uint8_t value{ 0 };

    constexpr color() = default;
    constexpr color(int c): value{ static_cast<uint8_t>(c) } { }

    constexpr bool operator==(const color&) const = default;
};

template <> struct color<color_mode::DIRECT> {
    uint8_t r{ 0 }, g{ 0 }, b{ 0 };

    constexpr color() = default;
    constexpr color(uint8_t _r, uint8_t _g, uint8_t _b): r{ _r }, g{ _g }, b{ _b } { }

    constexpr color(srgb srgb)
        : r{ static_cast<uint8_t>(std::clamp(std::lround(srgb.r * 255), 0l, 255l)) },
          g{ static_cast<uint8_t>(std::clamp(std::lround(srgb.g * 255), 0l, 255l)) },
          b{ static_cast<uint8_t>(std::clamp(std::lround(srgb.b * 255), 0l, 255l)) } { }

    constexpr color(std::convertible_to<srgb> auto c): color{ static_cast<srgb>(c) } { }
    template <color_space CS> constexpr color(CS c) requires (not std::convertible_to<CS, srgb>): color{ srgb{ static_cast<ciexyz>(c) } } { }
};

color(uint8_t) -> color<XTERM_256COLOR>;
color(uint8_t, uint8_t, uint8_t) -> color<DIRECT>;
color(color_space auto) -> color<DIRECT>;

inline constexpr color black{ 0 };
inline constexpr color red{ 1 };
inline constexpr color green{ 2 };
inline constexpr color yellow{ 3 };
inline constexpr color blue{ 4 };
inline constexpr color magenta{ 5 };
inline constexpr color cyan{ 6 };
inline constexpr color white{ 7 };
inline constexpr color reset_color{ 9 };

enum class underline_kind {
    NONE = -1,
    SINGLE = 0, // this type should zero-initialize to SINGLE underline
    DOUBLE = 1,
    CURLY = 2,
    DOTTED = 3,
    DASHED = 4
};

inline constexpr aspect_generator<^^color, "fg"> $fg;
inline constexpr aspect_generator<^^color, "bg"> $bg;

inline constexpr aspect_generator<^^underline_kind, "underline"> $underline;
inline constexpr aspect_generator<^^color, "underline_color"> $underline_color;
inline constexpr aspect_generator<^^bool, "italic"> $italic;
inline constexpr aspect_generator<^^bool, "bold"> $bold;
inline constexpr aspect_generator<^^bool, "overline"> $overline; // not always supported
inline constexpr aspect_generator<^^bool, "strikethrough"> $strikethrough;
inline constexpr aspect_generator<^^bool, "blink"> $blink; // not always supported
inline constexpr aspect_generator<^^bool, "invert"> $invert;

template <class... A> struct text: public element<text<A...>> {
    using el_t = element<text<A...>>;

    constexpr text() = default;
    constexpr text(A... a): element<text<A...>>{ std::move(a)... } { }

    constexpr void format(this const text& a, std::format_context& ctx) {
        auto format_escape = [&ctx, first = true]<class ...T>(std::format_string<T...> fmt, T&& ...args) mutable {
            if (!first) {
                ctx.advance_to(std::format_to(ctx.out(), ";"));
            }
            ctx.advance_to(std::format_to(ctx.out(), fmt, std::forward<T>(args)...));
            first = false;
        };
        
        a.template map<$fg>([&](color<XTERM_256COLOR> c) {
            if (c == reset_color) {
                format_escape("39");
            } else {
                format_escape("38;5;{}", c.value);
            }
        });
        a.template map<$fg>([&](color<DIRECT> c) {
            format_escape("38;2;{};{};{}", c.r, c.g, c.b);
        });

        a.template map<$bg>([&](color<XTERM_256COLOR> c) {
            format_escape("48;5;{}", c.value);
        });
        a.template map<$bg>([&](color<DIRECT> c) {
            format_escape("48;2;{};{};{}", c.r, c.g, c.b);
        });

        a.template map<$underline>([&](underline_kind kind) {
            format_escape("4:{}", static_cast<int>(kind) + 1);
        });
        a.template map<$underline_color>([&](color<XTERM_256COLOR> c) {
            if (c == reset_color) {
                format_escape("59");
            } else {
                format_escape("58;5;{}", c.value);
            }
        });
        a.template map<$underline_color>([&](color<DIRECT> c) {
            format_escape("58;2;{};{};{}", c.r, c.g, c.b);
        });

        a.template map<$bold>([&](bool enable) {
            format_escape("{}", enable ? 1 : 22);
        });
        a.template map<$italic>([&](bool enable) {
            format_escape("{}", enable ? 3 : 23);
        });
        a.template map<$overline>([&](bool enable) {
            format_escape("{}", enable ? 53 : 55);
        });
        a.template map<$strikethrough>([&](bool enable) {
            format_escape("{}", enable ? 9 : 29);
        });
        a.template map<$blink>([&](bool enable) {
            format_escape("{}", enable ? 5 : 25);
        });
        a.template map<$invert>([&](bool enable) {
            format_escape("{}", enable ? 7 : 27);
        });
    }

    constexpr auto format_prefix(std::format_context& ctx) const {
        if constexpr (text::count_aspects() >= 1) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[", ESC));

            this->format(ctx);
            ctx.advance_to(std::format_to(ctx.out(), "m"));
        }
    }

    constexpr auto format_suffix(std::format_context& ctx) const {
        if constexpr (text::count_aspects() >= 1) { ctx.advance_to(std::format_to(ctx.out(), "{}[0m", ESC)); }
    }
};


inline constexpr aspect_generator<^^int, "up"> $up;
inline constexpr aspect_generator<^^int, "down"> $down;
inline constexpr aspect_generator<^^int, "forward"> $forward;
inline constexpr aspect_generator<^^int, "back"> $back;

inline constexpr aspect_generator<^^int, "row"> $row;
inline constexpr aspect_generator<^^int, "col"> $col;

enum erase_kind {
    AFTER = 2,  // actual code: 0
    BEFORE = 1, // 1
    ALL = 0     // 2, but we want to zero-initialize to this
};

inline constexpr aspect_generator<^^erase_kind, "erase_display"> $erase_display;
inline constexpr aspect_generator<^^erase_kind, "erase_line"> $erase_line;

template <aspect_type... A> struct cursor: public element<cursor<A...>> {
    constexpr cursor() = default;

    constexpr cursor(A&&... a): element<cursor<A...>>{ std::forward<A>(a)... } { }

    constexpr auto operator()(this auto& self, std::format_context& ctx) {
        if constexpr (self.has($row) && self.has($col)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{};{}H", ESC, self[$row], self[$col]));
        } else if constexpr (self.has($col)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}G", ESC, self[$col]));
        }

        if constexpr (self.has($up)) { ctx.advance_to(std::format_to(ctx.out(), "{}[{}A", ESC, self[$up])); }

        if constexpr (self.has($down)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}B", ESC, self[$down]));
        }

        if constexpr (self.has($forward)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}C", ESC, self[$forward]));
        }

        if constexpr (self.has($back)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}D", ESC, self[$back]));
        }

        // after positioning, see if we need to erase

        if constexpr (self.has($erase_display)) {
            ctx.advance_to(std::format_to(
                ctx.out(), "{}[{}J", ESC, 2 - static_cast<unsigned int>(self[$erase_display])));
        }

        if constexpr (self.has($erase_line)) {
            ctx.advance_to(
                std::format_to(ctx.out(), "{}[{}K", ESC, 2 - static_cast<unsigned int>(self[$erase_line])));
        }
    }
};

template <aspect_type... A> struct scroll: public element<scroll<A...>> {
    constexpr scroll() = default;

    constexpr scroll(A&&... a): element<scroll<A...>>{ std::move(a)... } { }

    constexpr auto operator()(this auto& self, std::format_context& ctx) {
        if constexpr (self.has($up)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}S", ESC, self[$up]));
        }
        if constexpr (self.has($down)) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[{}T", ESC, self[$down]));
        }
    }
};

struct dec_private_mode {
    int Ps;
};

struct decset: dec_private_mode { };
struct decrst: dec_private_mode { };

constexpr dec_private_mode blinking_cursor{ 12 };
constexpr dec_private_mode show_cursor{ 25 };
constexpr dec_private_mode mouse_highlight_tracking{ 1001 };
constexpr dec_private_mode mouse_cell_motion_tracking{ 1002 };
constexpr dec_private_mode mouse_any_event_tracking{ 1003 };
constexpr dec_private_mode mouse_focus_event_tracking{ 1004 };
constexpr dec_private_mode mouse_utf8_encoding{ 1005 };
constexpr dec_private_mode mouse_sgr_encoding{ 1006 };
constexpr dec_private_mode mouse_sgr_pixel_encoding{ 1016 };
constexpr dec_private_mode save_cursor{ 1048 };
constexpr dec_private_mode alternate_screen_buffer{ 1049 };
constexpr dec_private_mode bracketed_paste{ 2004 };

inline constexpr aspect_generator<^^std::string_view, "icon_name"> $icon_name;
inline constexpr aspect_generator<^^std::string_view, "window_name"> $window_name;

struct x_window_property {
    std::string_view prop;
    std::string_view value;
};

inline constexpr aspect_generator<^^x_window_property, "xprop"> $xprop;

struct xtwinops {
    std::array<int, 3> Ps;

    template <class ...T> constexpr xtwinops(T... t): Ps{ t... } { }

    constexpr auto operator==(const xtwinops& other) const {
        return std::ranges::equal(Ps, other.Ps);
    }

    static constexpr xtwinops parse(const std::string_view sv) {
        std::istringstream ss{ sv };
        xtwinops hint{ };
        
        ss.ignore(2);
        for (int i = 0; i < 3; ++i) {
            ss >> hint.Ps[i];
            if (ss.peek() == ';') {
                ss.ignore(1);
                continue;
            } else {
                break;
            }
        }
        return hint;
    }
};

constexpr xtwinops window_position{ 13 };
constexpr xtwinops text_area_position{ 13, 2 };
constexpr xtwinops text_area_size_px{ 14 };
constexpr xtwinops window_size_px{ 14, 2 };
constexpr xtwinops screen_size_px{ 15 };
constexpr xtwinops cell_size_px{ 16 };
constexpr xtwinops text_area_size_cells{ 18 };
constexpr xtwinops screen_size_cells{ 19 };

template <std::output_iterator<char> It> constexpr auto strip_copy(const std::string_view sv, It out) {
    static constexpr std::array sequence_ends = { 'm', 'A', 'B', 'C', 'D', 'H', 'J', 'K', 't', 'l', 'h' };
    for (auto it = sv.begin(); it != sv.end(); ++it) {
        if (*it == ESC[0] && ++it != sv.end() && *it == '[') {
            it = std::find_first_of(it, sv.end(), sequence_ends.begin(), sequence_ends.end());
        } else {
            *(out++) = *it;
        }
    }
    return out;
}
template <class T> struct tty_formatter;

template <class ...A> struct tty_formatter<text<A...>> {
    constexpr auto format(const text<A...>& text, std::format_context& ctx) const {
        template for (const auto& c : text.children()) {
            text.format_prefix(ctx);
            ctx.advance_to(std::format_to(ctx.out(), "{}", c));
            text.format_suffix(ctx);
        }
        return ctx.out();
    }
};

template <std::derived_from<dec_private_mode> T> struct tty_formatter<T> {
    constexpr auto format(const decset& mode, std::format_context& ctx) const {
        if constexpr (std::same_as<T, decset>) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[?{}h", ESC, mode.Ps));
        } else if constexpr (std::same_as<T, decrst>) {
            ctx.advance_to(std::format_to(ctx.out(), "{}[?{}l", ESC, mode.Ps));
        }
        return ctx.out();
    }
};

template <> struct tty_formatter<xtwinops> {
    constexpr auto format(const xtwinops& hint, std::format_context& ctx) const {
        ctx.advance_to(std::format_to(ctx.out(), "{}[{}", tty::ESC, hint.Ps[0]));
        if (hint.Ps[1] != 0) {
            ctx.advance_to(std::format_to(ctx.out(), ";{}", hint.Ps[1]));
            if (hint.Ps[2] != 0) {
                ctx.advance_to(std::format_to(ctx.out(), ";{}", hint.Ps[2]));
            }
        }
        ctx.advance_to(std::format_to(ctx.out(), "t"));
        return ctx.out();
    }
};

template <class T, class Gen> struct tty_formatter<aspect<T, Gen>> {
    constexpr auto format(const aspect<T, Gen>& aspect, std::format_context& ctx) const {
        ctx.advance_to(std::format_to(ctx.out(), "${} = {}{}", text{ aspect.name(), $fg = 1 }, aspect.value, text{ ": "sv, display_string_of(^^T), $fg = 250 }));
        return ctx.out(); 
    }
};

template <class T> concept tty_formattable = requires (const T& t, std::format_context& ctx) {
    { tty_formatter<T>{ }.format(t, ctx) } -> std::convertible_to<decltype(ctx.out())>;
};

}

template <class ...A> struct std::formatter<tty::text<A...>> {
    tty::tty_formatter<tty::text<A...>> f;
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const tty::text<A...>& t, std::format_context& ctx) const {
        return f.format(t, ctx);
    }

};

template <class ...A> struct std::formatter<tty::cursor<A...>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const tty::cursor<A...>& c, std::format_context& ctx) const {
        c(ctx);
        return ctx.out();
    }

};

template <class ...A> struct std::formatter<tty::scroll<A...>> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const tty::scroll<A...>& s, std::format_context& ctx) const {
        s(ctx);
        return ctx.out();
    }

};


template <tty::tty_formattable T> struct std::formatter<T> {
    tty::tty_formatter<T> f;
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    constexpr auto format(const T& t, std::format_context& ctx) const {
        return f.format(t, ctx);
    }
};

#endif

