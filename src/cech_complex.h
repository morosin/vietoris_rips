#ifndef CECH_COMPLEX_H
#define CECH_COMPLEX_H

#include "Eigen/Core"
#include "simplicial_complex.h"

template <metric_space Space> struct cech_complex: point_cloud_complex<Space> {
    using metric_space_type = point_cloud_complex<Space>::metric_space_type;
    using point_type = point_cloud_complex<Space>::point_type;
    using scalar_type = point_cloud_complex<Space>::scalar_type;

    template <class R> constexpr cech_complex(R&& points, scalar_type epsilon)
        : point_cloud_complex<Space>{ std::forward<R>(points), epsilon } { }

    constexpr cech_complex(const cech_complex&) = delete;
    constexpr cech_complex(cech_complex&&) = default;

    constexpr bool intersection(const Eigen::ArrayXi& points) const {
        // calculate the distance from each point to the centroid of the hypothetical face. the centroid
        // should have minimal distance to each of the points, and would have to be in the `n`-fold
        // intersection of balls centered at these points if such an intersection is nonempty.

        auto coords = this->_points(points, Eigen::placeholders::all);
        auto centroid = coords.colwise().mean();
        return ((coords.rowwise() - centroid).rowwise().squaredNorm() <= this->epsilon_squared()).all();
    }
};

static_assert(nerve_complex<cech_complex<RR2>>);

#endif

