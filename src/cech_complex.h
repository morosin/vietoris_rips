#ifndef CECH_COMPLEX_H
#define CECH_COMPLEX_H

#include "Eigen/Core"
#include "simplicial_complex.h"
#include "matrix_formatters.h"
#include <print>
#include <ranges>

template <metric_space Space> struct cech_complex: point_cloud_complex<Space> {
    using metric_space_type = point_cloud_complex<Space>::metric_space_type;
    using point_type = point_cloud_complex<Space>::point_type;
    using scalar_type = point_cloud_complex<Space>::scalar_type;

    template <class R> constexpr cech_complex(R&& points, scalar_type epsilon): point_cloud_complex<Space>{ std::forward<R>(points), epsilon } { }
    
    constexpr cech_complex(const cech_complex&) = delete;
    constexpr cech_complex(cech_complex&&) = default;
 
    constexpr auto compute_skeleton(int n) {
            if (n == 0) {
                // the 0-chains are just points
                return chain_group<>{ 0,  std::views::iota(0, (int) this->n_points()) | std::views::transform([&](int i){
                    return simplex<>(i, std::array{ i }, std::array{ i });
                }) }; 
            }

        // here, an edge refers to an `n-1`-chain from which we form faces (`n`-chains)
        const chain_group<>& edges = this->skeleton(n-1);
        std::vector<simplex<>> faces{ };
        int added_faces{ };

        for (int edge_index = 0; edge_index < edges.rank(); ++edge_index) {
            auto& edge = edges.generators()[edge_index];
            auto vertices = this->points_of(edge);
            // std::println("edge {} matched to vertices {}", edge_index, vertices.matrix());

            // the points in `edge` should be ordered, so we can begin searching for new points with which to form a face after
            // all the already-included points. this should save time while also enforcing ordering.
            for (int j = edge.points().matrix().maxCoeff(); j < this->n_points(); ++j) {
                // std::println("checking for intersection with {}", j);
                Eigen::Array<int, Eigen::Dynamic, 1> p(n + 1);
                p(Eigen::seq(0, n - 1)) = edge.points();
                p(n) = j;

                // calculate the distance from each point to the centroid of the hypothetical face. the centroid should have minimal 
                // distance to each of the points, and would have to be in the `n`-fold intersection of balls centered at these points
                // if such an intersection is nonempty.
                auto coords = this->_points(p, Eigen::placeholders::all);
                auto centroid = coords.colwise().mean();
                auto dist = (coords.rowwise() - centroid).eval();

                if ((dist.rowwise().squaredNorm() <= this->epsilon_squared()).all()) {
                    auto [matched_edges, _, e] = this->make_simplex_arrays(n, edge, j);
                    if (matched_edges == n + 1) {
                        faces.emplace_back(simplex<>{ added_faces++, e, p });
                        // std::println("completed face {} with points {}", e.matrix(), p.matrix());
                    }
                }
            }
        }
        return chain_group<>{ (int) edges.rank(), std::move(faces) };
        // skeleta.emplace_back(chain_group<>{ (int) edges.rank(), faces });
    }

};

static_assert(simplicial_complex<cech_complex<RR2>>);

#endif

