#ifndef SIMPLICIAL_COMPLEX_H
#define SIMPLICIAL_COMPLEX_H

#include "Eigen/Core"
#include <concepts>
#include <meta>
#include <ranges>

template <class S> concept metric_space = requires(S s) {
    typename S::vector_type;
    typename S::scalar_type;
    {
        S::distance(std::declval<typename S::vector_type>(), std::declval<typename S::vector_type>)
    } -> std::convertible_to<typename S::scalar_type>;
};

template <class Scalar, std::size_t Dim> struct euclidean_space {
    using vector_type = Eigen::Vector<Scalar, Dim>;
    using scalar_type = Scalar;

    static constexpr auto dimension() { return Dim; }

    static constexpr auto distance(const vector_type& v1, const vector_type& v2) { return (v2 - v1).norm(); }

    static constexpr auto distance_squared(const vector_type& v1, const vector_type& v2) {
        return (v2 - v1).squaredNorm();
    }
};

using RR2 = euclidean_space<double, 2>;

template <auto Extent = Eigen::Dynamic> struct simplex {
    using storage_type = Eigen::Array<int, Extent == Eigen::Dynamic ? Eigen::Dynamic : Extent + 1, 1>;

private:
    storage_type _edges; // indices of codimension 1 subsimplices
    storage_type _points; // indices of points in the simplex (technically redundant but idk)
    int _index; // position in the list of n-simplices

public:
    constexpr simplex(): _index{ -1 }, _edges{ }, _points{ } { }
    template <std::ranges::sized_range R, std::ranges::sized_range S> constexpr simplex(int index, R&& edges, S&& points): _index{ index }, _edges(std::ranges::size(edges)), _points(std::ranges::size(points)) {
        std::ranges::copy(edges, _edges.begin());
        std::ranges::copy(points, _points.begin());
    }

    template <class DerivedE, class DerivedP> constexpr simplex(int index, const Eigen::ArrayBase<DerivedE>& edges, const Eigen::ArrayBase<DerivedP>& points): _index{ index }, _edges{ edges }, _points{ points } { }

    constexpr auto dimension() const { return _edges.rows(); }

    constexpr explicit(false) operator storage_type&(this auto& self) { return self._edges; }

    constexpr auto& edges(this auto& self) { return self._edges; }

    constexpr auto& points(this auto& self) { return self._points; }

    constexpr auto& operator[](this auto& self, std::size_t i) { return self._edges(i); }

    constexpr auto index() const {
        return _index;
    }
};

template <int Dim> struct chain_group {
    using boundary_type = Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>;
    using simplex_type = simplex<Dim>;

private:
    std::vector<simplex_type> _generators;
    boundary_type _boundary;

public:

    template <std::ranges::sized_range R> constexpr chain_group(int edge_rank, R&& r): _generators(std::ranges::size(r)), _boundary(boundary_type::Zero(edge_rank, std::ranges::size(r))) {
        static_assert(std::same_as<std::ranges::range_value_t<R>, simplex<Dim>>);
        auto it = _generators.begin();
         
        for (const auto& [idx, splx] : std::views::zip(std::views::iota(0), r)) {
            *it++ = splx; 
            for (int j = 0; j < splx.edges().rows(); ++j) { // the last one should be positive
                int sgn = (splx.edges().rows() - j) % 2;
                _boundary(splx[j], idx) = ((sgn == 0) ? -1 : 1);
            }
        }   
    }

    constexpr auto rank() const { return _generators.size(); }

    constexpr auto dimension() const { return Dim; }

    constexpr auto& generators() const { return _generators; }

    constexpr const boundary_type& boundary() const { return _boundary; }
};

namespace detail {
template <int N> consteval std::meta::info make_chain_groups_storage() {
    return []<int... I>(std::integer_sequence<int, I...>) {
        return ^^std::tuple<std::unique_ptr<chain_group<I>>...>;
    }(std::make_integer_sequence<int, N + 1>{});
}
} // namespace detail

template <int N> using chain_groups_storage = [:detail::make_chain_groups_storage<N>():];

template <class T> concept simplicial_complex = requires(T cplx, simplex<> s) {
    typename T::metric_space_type;
    typename T::point_type;
    typename T::scalar_type;

    { cplx.n_points() } -> std::integral;
    // { cplx.contains_simplex(s) } -> std::convertible_to<bool>;
    { cplx.template skeleton<0>() } -> std::convertible_to<chain_group<0>>;
};

#endif

