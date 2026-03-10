#ifndef VR_COMPLEX_H
#define VR_COMPLEX_H

#include "Eigen/Core"
#include "simplicial_complex.h"
#include <ranges>

template <class Space> struct vr_complex;

template <class Scalar, std::size_t Dim> struct vr_complex<euclidean_space<Scalar, Dim>> {
    using metric_space_type = euclidean_space<Scalar, Dim>;
    using point_type = metric_space_type::vector_type;
    using scalar_type = Scalar;
    static constexpr auto space_dimension = Dim;

private:
    using storage_type = Eigen::Array<Scalar, Eigen::Dynamic, Dim>; // each point is a row vector
    storage_type _points;
    scalar_type _epsilon;

public:
    template <std::ranges::range R> constexpr vr_complex(R&& points, scalar_type epsilon)
        : _points{ std::forward<R>(points) }, _epsilon{ epsilon } { }

    constexpr auto n_points() const { return _points.rows(); }

    template <std::ranges::view V> constexpr auto vertex_view(this auto& self, V&& indices) {
        return std::forward<V>(indices) | std::views::transform([&](std::size_t index) {
                   return self._points[index];
               });
    }

    constexpr auto epsilon() const { return _epsilon; }

    constexpr auto epsilon_squared(this auto& self) { return self.epsilon() * self.epsilon(); }

    template <index_list I> constexpr bool contains_simplex(this auto& self, const I& indices) {
        if (indices.size() > self.n_points()) { return false; }
        auto vertices = self._points(indices, Eigen::placeholders::all);

        for (std::size_t j = 1; j < indices.size(); ++j) {
            if (!((vertices.rowwise() - vertices(j, Eigen::placeholders::all)).squaredNorm()
                  <= self.epsilon_squared())
                     .all())
            {
                return false;
            }
        }
    }
};

#endif

