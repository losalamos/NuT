// types.hh
// T. M. Kelley
// Jan 03, 2011
// Header for types
// (c) Copyright 2011 LANSLLC all rights reserved.

#ifndef TYPES_H
#define TYPES_H


#include "RNG.hh"
#include "Vec3D.hh"
#include <utility>  // std::pair
#include <vector>
#include <string>

namespace nut
{
    namespace bdy_types
    {
        enum struct descriptor
        {
            V, /**< Vacuum */
            R, /**< Reflective */
            T  /**< Transmissive */
        };
    }

    typedef double geom_t;   /*^ type for geometry calculations */

    /** \brief Our type for geometric vectors */
    template <size_t dim> using vec_t = Vec_T<geom_t,dim>;

    typedef std::vector<geom_t> vg;

    typedef uint32_t cell_t; /*^ cell index. Use cell_t(-1) for null cell. */

    typedef uint32_t id_t;   /*^ particle index */

    typedef std::vector<id_t> vid;

    typedef uint32_t group_t;

    typedef uint32_t cntr_t;

    typedef nut::Philox4x32_RNG         rng_t;

    typedef rng_t::ctr_t                ctr_t;

    typedef rng_t::key_t                key_t;

    typedef rng_t::seed_t               seed_t;

    /* Neutrino species */
    enum Species
    {
          nu_e
        , nu_e_bar
        , nu_x
        , nu_mu
        , nu_mu_bar
        , nu_tau
        , nu_tau_bar
    }; // Species;

    std::string species_name( Species const s);

    Species species_type( std::string const & s);

    seed_t species_seed(Species const s);

} // nut::

#endif



// version
// $Id$

// End of file
