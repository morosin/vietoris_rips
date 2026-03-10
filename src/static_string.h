#ifndef STATIC_STRING_H
#define STATIC_STRING_H

#include <algorithm>
#include <array>
#include <string_view>

template <std::size_t N, class Char = char> struct static_string {
    std::array<Char, N> data;

    constexpr static_string() = default;

    constexpr explicit(false) static_string(const char (&_data)[N + 1]) {
        std::copy_n(_data, N, data.begin());
    }

    constexpr explicit(false) static_string(std::string_view sv) { std::copy_n(sv.begin(), N, data); }

    constexpr explicit(false) operator std::string_view() const {
        return std::string_view{ data.begin(), N };
    }

    template <class R> requires(std::same_as<std::ranges::range_value_t<R>, Char>)
    constexpr bool operator==(const R& range) const {
        return std::ranges::equal(static_cast<std::string_view>(*this), range);
    }

    constexpr std::size_t size() const { return N; }

    template <std::size_t M> constexpr static_string<N + M> operator+(const static_string<M>& rhs) const {
        static_string<N + M> r;
        auto it = std::copy_n(data.begin(), N, r.data.begin());
        std::copy_n(rhs.data.begin(), M, it);
    }
};

template <std::size_t N> static_string(const char (&_data)[N]) -> static_string<N - 1>;

#endif

