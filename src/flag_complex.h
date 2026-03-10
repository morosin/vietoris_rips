#ifndef FLAG_COMPLEX_H
#define FLAG_COMPLEX_H

#include "simplicial_complex.h"

template <simplicial_complex Cplx> struct flag_complex: Cplx {
    using base_complex = Cplx;
    using matric_space_type = Cplx::metric_space_type;
    using point_type = Cplx::point_type;
    using scalar_type = Cplx::scalar_type;
    static constexpr auto space_dimension = Cplx::space_dimension;

    using base_complex::base_complex;

    constexpr auto epsilon() const { return base_complex::epsilon() / 2; };

    template <index_list I> constexpr bool contains_simplex(this auto& self, const I& indices) {
        if (indices.size() > self.n_points()) { return false; }
        for (std::size_t j = 0; j < indices.size(); ++j) {
            for (std::size_t i = 0; i < j; ++i) {
                if (!static_cast<base_complex&>(self).contains_simplex({ indices[i], indices[j] })) {
                    return false;
                }
            }
        }
        return true;
    }
};

#endif

