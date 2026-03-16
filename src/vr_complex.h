#ifndef VR_COMPLEX_H
#define VR_COMPLEX_H

#include "cech_complex.h"
#include "flag_complex.h"

template <class Space> using vietoris_rips_complex = flag_complex<cech_complex<Space>>;
static_assert(simplicial_complex<vietoris_rips_complex<RR2>>);

#endif

