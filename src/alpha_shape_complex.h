#ifndef ALPHA_SHAPE_COMPLEX_H
#define ALPHA_SHAPE_COMPLEX_H

#include "Eigen/Core"
#include "simplicial_complex.h"

template <metric_space Space> struct alpha_shape_complex: point_cloud_complex<Space> {
    using metric_space_type = point_cloud_complex<Space>::metric_space_type;
    using point_type = point_cloud_complex<Space>::point_type;
    using scalar_type = point_cloud_complex<Space>::scalar_type;

    Eigen::MatrixX<scalar_type> distances;
    Eigen::MatrixXi neighborhoods;

    template <class R> constexpr alpha_shape_complex(R&& points, scalar_type epsilon)
        : point_cloud_complex<Space>{ std::forward<R>(points), epsilon } {

        // precompute the pairwise distances of all the points
        distances.resize(this->n_points(), this->n_points());
        neighborhoods.setIdentity(this->n_points(), this->n_points());
        Eigen::Index idx;
        for (int i = 0; i < this->n_points(); ++i) {
            distances(i, Eigen::placeholders::all) = (this->_points.rowwise()
                                                      - this->_points(i, Eigen::placeholders::all))
                                                         .rowwise()
                                                         .squaredNorm()
                                                         .transpose();
            distances(i, i) = std::numeric_limits<scalar_type>::infinity();
            auto dist = distances(i, Eigen::placeholders::all).minCoeff(&idx);
            neighborhoods(idx, i) = dist < this->epsilon_squared();
        }

        // row `idx` of `neighborhoods` has a 1 for each point that is closest to point `idx`
        // std::println("neighborhoods: {}", neighborhoods);
    }

    constexpr alpha_shape_complex(const alpha_shape_complex&) = delete;
    constexpr alpha_shape_complex(alpha_shape_complex&&) = default;

    constexpr void scale(this auto& self, scalar_type epsilon) {
        static_cast<point_cloud_complex<Space>&>(self).scale(epsilon);
        self.neighborhoods.setIdentity();
        Eigen::Index idx;
        for (int i = 0; i < self.n_points(); ++i) {
            auto dist = self.distances(i, Eigen::placeholders::all).minCoeff(&idx);
            self.neighborhoods(idx, i) = dist < self.epsilon_squared();
        }
    }

    constexpr bool intersection(const Eigen::ArrayXi& points) const {
        return neighborhoods(Eigen::placeholders::all, points).rowwise().all().any();
    }
};

static_assert(nerve_complex<alpha_shape_complex<RR2>>);

#endif

