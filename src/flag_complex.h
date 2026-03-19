#ifndef FLAG_COMPLEX_H
#define FLAG_COMPLEX_H

#include "Eigen/Core"
#include "simplicial_complex.h"
#include <print>

template <std::integral T> constexpr T binomial_coefficient(T n, T k) {
    return std::tgamma(n + 1) / (std::tgamma(k + 1) * std::tgamma(n - k + 1));
}

template <simplicial_complex Cplx> struct flag_complex: Cplx {
    using base_complex = Cplx;
    using metric_space_type = Cplx::metric_space_type;
    using point_type = Cplx::point_type;
    using scalar_type = Cplx::scalar_type;

private:
    Eigen::MatrixX<int> _adjacency;
    scalar_type _base_scale_factor;

public:
    // applies the flag construction to a given simplicial complex
    template <class... Args> constexpr flag_complex(scalar_type base_scale_factor, Args&&... args)
        : _base_scale_factor{ base_scale_factor }, base_complex{ std::forward<Args>(args)... } {

        _adjacency.setZero(this->n_points(), this->n_points());

        chain_group<>& edges = static_cast<base_complex&>(*this).skeleton(1);

        for (const simplex<>& edge : edges.generators()) { _adjacency(edge[0], edge[1]) = 1; }
    }

    constexpr flag_complex(const flag_complex&) = delete;
    constexpr flag_complex(flag_complex&&) = default;

    constexpr auto& adjacency_matrix() const { return _adjacency; }

    constexpr auto scale(this auto& self, scalar_type epsilon) {
        static_cast<base_complex&>(self).scale(epsilon / self._base_scale_factor);
        self._adjacency.setZero(self.n_points(), self.n_points());
        chain_group<>& edges = static_cast<base_complex&>(self).skeleton(1);

        for (const simplex<>& edge : edges.generators()) { self._adjacency(edge[0], edge[1]) = 1; }
    }
};

template <class Cplx> constexpr chain_group<> compute_skeleton(flag_complex<Cplx>& cplx, int n) {
    // for n=0 and n=1, we need the chain group from the underlying complex.
    if (n <= 1) { return compute_skeleton(static_cast<Cplx&>(cplx), n); }

    const auto& edges = cplx.skeleton(n - 1);
    std::vector<simplex<>> faces{};
    int added_faces{};

    for (const simplex<>& edge : edges.generators()) {
        Eigen::VectorXi point_indices = Eigen::VectorXi::Zero(cplx.n_points());
        point_indices(edge.points()).setConstant(1);

        for (int i = edge.points()[n - 1] + 1; i < cplx.n_points(); ++i) {

            // a vector with a 1 in each position for which the hypotheical face would contain that point, and
            // zeros elsewhere.
            point_indices(i) = 1;

            // the `k`th component of the `connect` vector is equal to the number 1-simplices based at point k
            // that lie in the hypothetical face.
            Eigen::ArrayXi connect = cplx.adjacency_matrix() * point_indices;
            // if we have the correct number of 1-simplices to form an `n`-simplex (i.e. n+1 choose 2) then
            // we're done
            if (connect.sum() >= binomial_coefficient(n + 1, 2)) {
                auto [matched_edges, p, e] = cplx.make_simplex_arrays(n, edge, i);
                if (matched_edges == n + 1) { // should always be true for a flag complex
                    faces.emplace_back(simplex<>{ added_faces++, e, p });
                }
            }
            point_indices(i) = 0;
        }
    }

    return chain_group<>{ static_cast<int>(edges.rank()), std::move(faces) };
}

#endif

