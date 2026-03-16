#ifndef ORDERED_SEQUENCES_VIEW_H
#define ORDERED_SEQUENCES_VIEW_H

#include <algorithm>
#include <cmath>
#include <ranges>

class ordered_subsequences_view: std::ranges::view_interface<ordered_subsequences_view> {
    struct iterator {
        std::size_t _seq_len;
        std::size_t* _array;
        std::size_t _subseq_len;
        std::size_t _n{ };

        constexpr auto operator*() const {
            return std::ranges::subrange(_array, _array + _subseq_len);
        }

        constexpr auto& operator++() {
            do {
                for (std::size_t i = 0; i < static_cast<std::size_t>(std::tgamma(_seq_len - _subseq_len + 1)); ++i) {
                    if (!std::next_permutation(_array, _array + _seq_len)) {
                        _array = nullptr;
                        return *this;
                    }
                }
            } while (!std::is_sorted(_array, _array + _subseq_len));
            ++_n;
            return *this;
        }

        constexpr auto& operator++(int) {
            return ++*this;
        }

        friend constexpr auto operator==(const iterator& lhs, const iterator& rhs){ return lhs._array == rhs._array; }
    };
    iterator _it;
public:
    template <std::ranges::sized_range R> constexpr ordered_subsequences_view(R&& sequence, std::size_t length): _it{ } {
        _it._seq_len = std::ranges::size(sequence);
        _it._array = new std::size_t[_it._seq_len];
        _it._subseq_len = length;
        std::ranges::copy(sequence, _it._array);
    }

    ordered_subsequences_view(ordered_subsequences_view&) = delete;
    ordered_subsequences_view(ordered_subsequences_view&&) = default;

    constexpr ~ordered_subsequences_view() {
        delete[] _it._array;
    }

    constexpr auto begin() const {
        return _it;
    }
    
    constexpr auto end() const {
        return iterator{ 0, nullptr, 0 };
    }
};


#endif

