#ifndef SIMPLICIAL_COMPLEX_H
#define SIMPLICIAL_COMPLEX_H

#include "Eigen/Core"
#include "Eigen/LU"
#include "Eigen/Sparse"
#include <algorithm>
#include <concepts>
#include <meta>
#include <ranges>

template <class S> concept metric_space = requires(S s) {
    typename S::vector_type;
    typename S::scalar_type;
    {
        (std::declval<typename S::vector_type>() - std::declval<typename S::vector_type>()).norm()
    } -> std::convertible_to<typename S::scalar_type>;
    { S::dimension() } -> std::integral;
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
    using storage_type = Eigen::Array<int, Extent, 1>;

private:
    storage_type _edges;  // indices of codimension 1 subsimplices
    storage_type _points; // indices of points in the simplex (technically redundant but idk)
    int _index;           // position in the list of n-simplices

public:
    template <std::ranges::sized_range R, std::ranges::sized_range S>
    constexpr simplex(int index, R&& edges, S&& points)
        : _index{ index }, _edges(std::ranges::size(edges)), _points(std::ranges::size(points)) {
        std::ranges::copy(edges, _edges.begin());
        std::ranges::copy(points, _points.begin());
    }

    template <class DerivedE, class DerivedP>
    constexpr simplex(int index, const Eigen::ArrayBase<DerivedE>& edges,
                      const Eigen::ArrayBase<DerivedP>& points)
        : _index{ index }, _edges{ edges }, _points{ points } { }

    constexpr auto dimension() const { return _edges.rows(); }

    constexpr explicit(false) operator storage_type&(this auto& self) { return self._edges; }

    constexpr auto& edges(this auto& self) { return self._edges; }

    constexpr auto& points(this auto& self) { return self._points; }

    // access the index of the `i`th edge
    constexpr auto& operator[](this auto& self, std::size_t i) { return self._edges(i); }

    // the index of this simplex in its chain group
    constexpr auto index() const { return _index; }

    constexpr std::strong_ordering operator<=>(const simplex<>& rhs) const {
        return std::lexicographical_compare_three_way(
            this->points().begin(), this->points().end(), rhs.points().begin(), rhs.points().end());
    }
};

template <int Deg = Eigen::Dynamic> struct chain_group {
    using simplex_type = simplex<Deg == Eigen::Dynamic ? Deg : Deg + 1>;

private:
    std::vector<simplex_type> _generators;
    Eigen::SparseMatrix<int> _boundary;
    std::optional<Eigen::SparseMatrix<int>> _bdy_kernel;
    std::optional<Eigen::SparseMatrix<int>> _bdy_image;

    // to find the homology, we want to cache the decomposition of the boundary matrix. we theoretically don't
    // need to save the whole matrices, but it's kind of fun to see the kernel if you want.
    constexpr void check_boundary_computations() {
        if (!_bdy_kernel.has_value()) {
            // the sparse solvers don't reliably calculate rank for integer matrices and don't reveal the
            // image or null space so we temporarily convert them to dense matrices
            const auto& bdy = _boundary.toDense();
            Eigen::FullPivLU<Eigen::MatrixXi> solver;
            solver.compute(bdy);
            _bdy_kernel = solver.kernel().sparseView();
            _bdy_image = solver.image(bdy).sparseView();
            _bdy_kernel->makeCompressed();
            _bdy_image->makeCompressed();
        }
    }

public:
    chain_group(const chain_group&) = delete;
    constexpr chain_group(chain_group&&) = default;

    template <std::ranges::range R> constexpr chain_group(int edge_rank, R&& gens)
        : _generators{ std::from_range, gens },
          _boundary(std::max(edge_rank, 1), std::max(_generators.size(), 1ul)) {
        if (edge_rank > 0 && _generators.size() > 0) {

            // form the boundary map
            std::vector<Eigen::Triplet<int>> triplets{
                std::from_range,
                _generators | std::views::transform([](const auto& splx) {
                    return std::views::zip(std::views::iota(0, splx.edges().rows()),
                                           std::views::repeat(splx));
                }) | std::views::join
                    | std::views::transform([](const auto& t) {
                          const auto& [edge_idx, splx] = t;
                          int sgn = (splx.edges().rows() - edge_idx) % 2;
                          return Eigen::Triplet<int>{ splx[edge_idx], splx.index(), ((sgn == 0) ? -1 : 1) };
                      })
            };
            _boundary.setFromTriplets(triplets.begin(), triplets.end());
        } else {
            _boundary.setZero();
        }
        _boundary.makeCompressed();
    }

    // the rank of the chain group (as a ℤ-module)
    constexpr auto rank() const { return _generators.size(); }

    // this is a group of `dimension()`-chains
    constexpr auto dimension() const { return Deg; }

    // the set of simplices that generate the group
    constexpr const auto& generators() const { return _generators; }

    constexpr const Eigen::SparseMatrix<int>& boundary() const { return _boundary; }

    constexpr auto& boundary_image() {
        check_boundary_computations();
        return *_bdy_image;
    }

    constexpr auto& boundary_kernel() {
        check_boundary_computations();
        return *_bdy_kernel;
    }

    constexpr int boundary_rank() {
        if (rank() > 0) {
            check_boundary_computations();
            return rank() - _bdy_kernel->cols();
        }
        return 0;
    }
};

template <class T> concept simplicial_complex = requires(T& cplx, simplex<> s, int n) {
    typename T::metric_space_type;
    typename T::point_type;
    typename T::scalar_type;

    { cplx.n_points() } -> std::integral;
    { cplx.skeleton(n) } -> std::convertible_to<chain_group<>&>;
    { cplx.points_of(s) } -> std::convertible_to<Eigen::MatrixX<typename T::scalar_type>>;
    { cplx.epsilon() } -> std::convertible_to<typename T::scalar_type>;
    { cplx.scale(std::declval<typename T::scalar_type>()) };
};

template <class T> concept nerve_complex = simplicial_complex<T> and requires(T cplx, Eigen::ArrayXi pts) {
    { cplx.intersection(pts) } -> std::convertible_to<bool>;
};

template <simplicial_complex Cplx> constexpr int betti_number(Cplx& cplx, int n) {
    chain_group<>& n_plus_one_chains = cplx.skeleton(n + 1);

    if (n == 0) { return cplx.n_points() - n_plus_one_chains.boundary_rank(); }
    chain_group<>& n_chains = cplx.skeleton(n);

    return n_chains.boundary_kernel().cols() - n_plus_one_chains.boundary_image().cols();
}

template <metric_space Space> struct point_cloud_complex {
protected:
    using metric_space_type = Space;
    using point_type = Space::vector_type;
    using scalar_type = Space::scalar_type;
    using storage_type = Eigen::Array<scalar_type, Eigen::Dynamic, Space::dimension()>;
    storage_type _points;
    scalar_type _epsilon;
    std::vector<std::unique_ptr<chain_group<>>> skeleta{};

    template <class R> constexpr point_cloud_complex(R&& points, scalar_type epsilon)
        : _points{ std::forward<R>(points) }, _epsilon{ epsilon } { }

public:
    // complexes shouldn't be copyable because their managed `skeleta` are allocated dynamically.
    constexpr point_cloud_complex(const point_cloud_complex&) = delete;
    constexpr point_cloud_complex(point_cloud_complex&&) = default;

    constexpr auto n_points() const { return _points.rows(); }

    constexpr auto& points() const { return _points; }

    constexpr auto epsilon() const { return _epsilon; }

    constexpr void scale(this auto& self, scalar_type epsilon) {
        self._epsilon = epsilon;
        std::ranges::destroy(self.skeleta);
        self.skeleta.clear();
    }

    constexpr auto epsilon_squared() const { return epsilon() * epsilon(); }

    template <int N> constexpr decltype(auto) points_of(this auto& self, const simplex<N> splx) {
        return self._points(splx.points(), Eigen::placeholders::all);
    }

    // constructs a new simplex from the points of an initial edge and one additional point.
    // currently searches though the `degree-1`-skeleton, which to me feels unnecessary but otherwise i'm
    // not sure how to determine the indices of the new simplex's constituent edges (since we just know their
    // points). assume that `degree >= 1`, the points of initial_edge are in order, and that `point` is
    // strictly greater than all of them.
    constexpr auto make_simplex_arrays(this auto& self, int degree, const simplex<>& initial_edge,
                                       int point) {
        const auto& edges = self.skeleton(degree - 1);

        Eigen::ArrayXi p(degree + 1);
        Eigen::ArrayXi e(degree + 1);
        p(Eigen::seq(0, degree - 1)) = initial_edge.points();
        p(degree) = point;
        auto edges_range = e(Eigen::seq(0, degree));
        auto [_, result] = std::ranges::copy(edges.generators() | std::views::filter([&](const auto& gen) {
                                                 return std::ranges::includes(
                                                     p(Eigen::seq(0, degree)),
                                                     gen.points()(Eigen::seq(0, degree - 1)));
                                             }) | std::views::transform([](const auto& gen) {
                                                 return gen.index();
                                             }),
                                             edges_range.begin());

        return std::make_tuple(std::distance(edges_range.begin(), result), std::move(p), std::move(e));
    }

    // retrieves the group of `n`-chains if it has already been computed, or uses the `compute_skeleton`
    // function of a derived class to compute it.
    template <std::derived_from<point_cloud_complex> Self>
    constexpr chain_group<>& skeleton(this Self& self, int n) {
        if (n >= self.computed_skeleta()) {
            self.skeleta.emplace_back(std::make_unique<chain_group<>>(compute_skeleton(self, n)));
        }
        return *self.skeleta[n];
    }

    constexpr auto computed_skeleta() const { return skeleta.size(); }
};

constexpr auto compute_skeleton(nerve_complex auto& cplx, int n) {
    if (n == 0) {
        // the 0-chains are just points
        return chain_group<>{ 0,
                              std::views::iota(0, (int)cplx.n_points()) | std::views::transform([&](int i) {
                                  return simplex<>(i, std::array{ i }, std::array{ i });
                              }) };
    }

    // here, an edge refers to an `n-1`-chain from which we form faces (`n`-chains)
    const chain_group<>& edges = cplx.skeleton(n - 1);
    std::vector<simplex<>> faces{};
    int added_faces{};

    for (int edge_index = 0; edge_index < edges.rank(); ++edge_index) {
        auto& edge = edges.generators()[edge_index];
        auto vertices = cplx.points_of(edge);
        // std::println("edge {} matched to vertices {}", edge_index, vertices.matrix());

        // the points in `edge` should be ordered, so we can begin searching for new points with which to form
        // a face after all the already-included points. this should save time while also enforcing ordering.
        for (int j = edge.points()[n - 1] + 1; j < cplx.n_points(); ++j) {
            // std::println("checking for intersection with {}", j);
            Eigen::Array<int, Eigen::Dynamic, 1> p(n + 1);
            p(Eigen::seq(0, n - 1)) = edge.points();
            p(n) = j;

            if (cplx.intersection(p)) {
                auto [matched_edges, _, e] = cplx.make_simplex_arrays(n, edge, j);
                if (matched_edges == n + 1) {
                    faces.emplace_back(simplex<>{ added_faces++, e, p });
                    // std::println("completed face {} with points {}", e.matrix(), p.matrix());
                }
            }
        }
    }
    return chain_group<>{ (int)edges.rank(), std::move(faces) };
}

#endif

