#ifndef CECH_COMPLEX_H
#define CECH_COMPLEX_H

#include "simplicial_complex.h"
#include <memory_resource>
#include <print>
#include <ranges>

template <class Space> struct cech_complex;

template <class Scalar, std::size_t Dim> struct cech_complex<euclidean_space<Scalar, Dim>> {
    using metric_space_type = euclidean_space<Scalar, Dim>;
    using point_type = metric_space_type::vector_type;
    using scalar_type = Scalar;
    static constexpr auto space_dimension = Dim;

private:
    using storage_type = Eigen::Array<Scalar, Eigen::Dynamic, Dim>; // each point is a row vector
    storage_type _points;
    scalar_type _epsilon;

    std::pmr::monotonic_buffer_resource memory_pool;
    std::pmr::polymorphic_allocator<> chain_allocator{ &memory_pool };
    std::vector<void*> skeleta;

public:
    template <class R> constexpr cech_complex(R&& points, scalar_type epsilon)
        : _points{ std::forward<R>(points) }, _epsilon{ epsilon } { }

    constexpr cech_complex(const cech_complex&) = delete;
    constexpr cech_complex(cech_complex&&) = default;

    constexpr auto n_points() const { return _points.rows(); }

    constexpr auto& points() const { return _points; }

    constexpr auto epsilon() const { return _epsilon; }

    constexpr auto epsilon_squared(this auto& self) { return self.epsilon() * self.epsilon(); }

    template <int N> constexpr decltype(auto) points_of(this auto& self, const simplex<N>& splx) {
        return self._points(splx.points(), Eigen::placeholders::all);
    }

    template <int N> constexpr auto& skeleton(this auto& self) {
        if (self.skeleta.size() <= N) {
            if constexpr (N == 0) {
                self.skeleta.push_back(self.chain_allocator.template new_object<chain_group<0>>(
                    self.n_points(),
                    std::views::iota(0, (int)self.n_points()) | std::views::transform([](int i) {
                        return simplex<0>{ i, std::array{ i }, std::array{ i } };
                    })));
            } else {
                auto& edges = self.template skeleton<N - 1>();
                std::vector<simplex<N>> faces;

                int added_faces{};

                for (int edge_index = 0; edge_index < edges.rank(); ++edge_index) {
                    auto& edge = edges.generators()[edge_index];
                    auto vertices = self.points_of(edge);
                    // std::println("edge {} matched to vertices {}", edge_index, vertices.matrix());

                    for (int j = 0; j < self.n_points(); ++j) {
                        if ((edge.points() >= j).any()) { continue; }
                        // std::println("checking for intersection with {}", j);
                        auto centroid = vertices.colwise().mean();
                        Eigen::Array<int, N + 1, 1> p;
                        p(Eigen::seq(0, N - 1)) = edge.points();
                        p(N) = j;

                        // Eigen::Matrix<Scalar, N + 1, Dim> coords{ };
                        auto coords = self._points(p, Eigen::placeholders::all);

                        auto dist = coords.rowwise() - centroid;
                        // std::println("dist: {}", (dist.rowwise().squaredNorm() - (self.epsilon_squared()
                        // / 4.0)).matrix());
                        if ((dist.rowwise().squaredNorm() <= (self.epsilon_squared() / 4.0)).all()) {
                            Eigen::Array<int, N + 1, 1> e;
                            auto edges_range = e(Eigen::seq(0, N));
                            auto [_, result] = std::ranges::copy(
                                edges.generators() | std::views::filter([&p](const auto& gen) {
                                    return std::ranges::includes(p(Eigen::seq(0, N)),
                                                                 gen.points()(Eigen::seq(0, N - 1)));
                                }) | std::views::transform([](const auto& gen) {
                                    return gen.index();
                                }),
                                edges_range.begin());
                            if (std::distance(edges_range.begin(), result) == N + 1) {
                                faces.emplace_back(added_faces++, e, p);
                                // std::println("completed face {} with points {}", e.matrix(), p.matrix());
                            }
                        }
                    }
                }
                self.skeleta.push_back(
                    self.chain_allocator.template new_object<chain_group<N>>(edges.rank(), std::move(faces)));
            }
        }

        return *static_cast<chain_group<N>*>(self.skeleta[N]);
    }
};

static_assert(simplicial_complex<cech_complex<RR2>>);

#endif

