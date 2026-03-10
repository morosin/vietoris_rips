#ifndef INDEX_SEQUENCE_POLYFILL_H
#define INDEX_SEQUENCE_POLYFILL_H

#if __cpp_lib_integer_sequence < 202511L

#include <utility>
namespace std {

template <class T, T ...V> struct tuple_size<integer_sequence<T, V...>>: integral_constant<size_t, sizeof...(V)> { };
template <size_t I, class T, T ...V> requires (I < sizeof...(V)) struct tuple_element<I, integer_sequence<T, V...>> {
    using type = T;
};
template <size_t I, class T, T ...V> requires (I < sizeof...(V)) constexpr T get(integer_sequence<T, V...>) noexcept {
    return V...[I];
}

}
#endif

#endif
