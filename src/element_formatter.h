#ifndef ELEMENT_FORMATTER_H
#define ELEMENT_FORMATTER_H

#include "element.h"
#include "tty.h"

template <element_type T> struct std::formatter<T> {
    int depth{};

    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    constexpr auto indent(int n) const { return std::string(4 * (depth + n), ' '); }

    constexpr auto format(const T& el, std::format_context& ctx) const {
        using namespace tty;

        if constexpr (std::is_invocable_v<const T&, std::format_context&>) {
            std::invoke(el, ctx);
        } else {
            if consteval {
                ctx.advance_to(
                    std::format_to(ctx.out(), "{}{}", indent(0), text{ display_string_of(^^T), $fg = 3 }));
            } else {
                // if we don't do this, the bit_cast will make this unusable during constant evaluation
                ctx.advance_to(std::format_to(ctx.out(),
                                              "{}{} @ {:#010x}",
                                              indent(0),
                                              text{ display_string_of(^^T), $fg = 3 },
                                              std::bit_cast<intptr_t>(std::addressof(el))));
            }

            template for (aspect_type auto& a : el.aspects()) {
                std::formatter<std::decay_t<decltype(a)>> af{};
                ctx.advance_to(std::format_to(ctx.out(), "\n{}", indent(1)));
                af.format(a, ctx);
            }

            template for (const auto& c : el.children()) {
                ctx.advance_to(std::format_to(ctx.out(), "\n"));

                using U = std::decay_t<decltype(c)>;

                if constexpr (element_type<U>) {
                    std::formatter<U> f{ depth + 1 };
                    f.format(c, ctx);
                } else {
                    ctx.advance_to(std::format_to(ctx.out(), "{}{}", indent(1), c));
                }
            }
        }
        return ctx.out();
    };
};

#endif

