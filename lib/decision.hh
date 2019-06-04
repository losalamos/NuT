// decision.hh
// T. M. Kelley
// Jan 24, 2011
// (c) Copyright 2011 LANSLLC, all rights reserved

#ifndef DECISION_HH
#define DECISION_HH

#include "constants.hh"
#include "events.hh"
#include "lorentz.hh"
#include "types.hh"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>  // pair
// #include <iomanip>

/* unresolved_event is not used in all translation units,
 * leading to unexciting warnings. Suppress those. */
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"

namespace nut {

namespace {
/** throw an exception when code fails to decide an event */
void
unresolved_event(size_t lineno, std::string const & eventstr);

/** Compare std::pairs on the second element. */
template <typename pair>
bool
pair_min_2nd(pair const & p1, pair const & p2)
{
  return p1.second < p2.second;
}

}  // namespace

template <typename particle_t,
          typename mesh_t,
          typename opacity_t,
          typename velocity_t>
event_n_dist
decide_event(particle_t & p,  // non-const b/c of RNG
             mesh_t const & mesh,
             opacity_t const & opacity,
             velocity_t const & velocity)
{
  constexpr size_t dim(particle_t::dim);

  typedef typename particle_t::fp_t fp_t;
  // Currently there are always  three top-level events considered.
  // Changing vector to std::array
  typedef std::array<event_n_dist, 3> vend_t;

  vend_t e_n_ds;

  cell_t const cell = p.cell;
  fp_t const tleft = p.t;
  vec_t<dim> const x = p.x;
  Species const species = p.species;

  // compute distance to events, push onto vector
  // compute cross-section in comoving frame

  vec_t<dim> v = velocity.v(cell);
  // TO DO vector velocity
  vec_t<dim> vtmp(v);
  geom_t const eli = p.e;
  vec_t<dim> const oli = p.omega;
  // LT to comoving frame (compute interaction comoving).
  EandOmega<dim> eno_cmi = mesh_t::LT_to_comoving(vtmp, eli, oli);
  geom_t const eci = eno_cmi.first;

  fp_t const sig_coll = opacity.sigma_collide(cell, eci, species);

  fp_t const random_dev = p.rng.random();
  /* fp_t const ignored    =*/p.rng.random();  // to keep pace with McPhD

  // collision distance
  geom_t const d_coll =
      (sig_coll != fp_t(0)) ? -std::log(random_dev) / sig_coll : huge;

  e_n_ds[0] = event_n_dist(events::collision, d_coll);

  // boundary distance
  typename mesh_t::Intersection dnf = mesh.distance_to_bdy(x, oli, cell);
  geom_t const d_bdy = mesh.get_distance(dnf);
  typename mesh_t::Face face = mesh.get_face(dnf);
  e_n_ds[1] = event_n_dist(events::boundary, d_bdy);

  // time step end distance
  geom_t const d_step_end = c * tleft;
  e_n_ds[2] = event_n_dist(events::step_end, d_step_end);

  // pick (event,dist) pair with shortest distance
  event_n_dist closest = *(std::min_element(e_n_ds.begin(), e_n_ds.end(),
                                            pair_min_2nd<event_n_dist>));

  // if needed, further resolve events
  if(closest.first == events::collision) {
    // need to compute the comoving energy at the scattering site,
    // in order to compute the different cross sections there.

    typename mesh_t::coord_t const scat_site(
        mesh.new_coordinate(x, oli, d_coll));
    vec_t<dim> const o_sct = scat_site.omega;
    EandOmega<dim> const eno_scat = mesh_t::LT_to_comoving(vtmp, eli, o_sct);
    fp_t const ecscat = eno_scat.first;
    closest.first = decide_scatter_event(p.rng, ecscat, cell, opacity, species);
  }
  else {
    if(closest.first == events::boundary) {
      closest.first = decide_boundary_event(mesh, cell, face);
    }
    // compensate RNG if decide_scatter_event not called
    p.rng.random();
  }
  return closest;
}  // decide_event

/**\brief Examine the cell & face, and determine what kind of boundary this is.
 * \remark If it is a cell boundary, the face will be encoded in the event.
 * Decode via events::decode_event.
 */
template <typename MeshT>
events::Event
decide_boundary_event(MeshT const & mesh, cell_t const cell, cell_t const face)
{
  using namespace bdy_types;
  using namespace events;
  Event event(null);
  bdy_types::descriptor b_type(mesh.get_bdy_type(cell, face));
  switch(b_type) {
    case VACUUM: event = escape; break;
    case REFLECTIVE: event = reflect; break;
    case NONE:
      event = cell_boundary;
      event = events::encode_face(event, face);
      break;
    case PERIODIC:
    case PROCESSOR:
    default:
      std::stringstream errstr;
      errstr << "decide_boundary_event: untreated boundary " << b_type;
      unresolved_event(__LINE__, errstr.str());
  };
  return event;
}  // decide_boundary_event

template <typename rng_t, typename fp_t, typename opacity_t>
events::Event
decide_scatter_event(rng_t & rng,
                     fp_t const nu_nrg,
                     cell_t const cell,
                     opacity_t const & op,
                     Species const s)
{
  typedef const fp_t fp_c;
  // cell_t const idx = cell - 1;
  fp_c sig_collide = op.sigma_collide(cell, nu_nrg, s);

  // selectors
  fp_c p_type_sel = rng.random();  // particle interactor
  events::Event event(events::null);

  fp_c N_total = op.sigma_N_total(cell, nu_nrg);  // nucleon
  fp_c prob_nucleon = N_total / sig_collide;

  // other top-level cross-sections here

  fp_c prob_abs = op.sigma_N_abs(cell, nu_nrg) / sig_collide;
  fp_c prob_elastic = op.sigma_N_elastic(cell, nu_nrg) / sig_collide;
  fp_c prob_electron = op.sigma_nu_e_minus(cell, nu_nrg, s) / sig_collide;
  fp_c prob_positron = op.sigma_nu_e_plus(cell, nu_nrg, s) / sig_collide;

  if(p_type_sel < prob_abs) { event = events::nucleon_abs; }
  else if(p_type_sel < prob_abs + prob_elastic) {
    event = events::nucleon_elastic_scatter;
  }
  else if(p_type_sel < prob_nucleon + prob_electron) {
    event = events::electron_scatter;
  }
  else if(p_type_sel < prob_nucleon + prob_electron + prob_positron) {
    event = events::positron_scatter;
  }
  else {
    unresolved_event(__LINE__, "scatter");
  }
  return event;
}  // decide_scatter_event

namespace {
void
unresolved_event(size_t lineno, std::string const & eventstr)
{
  std::stringstream msg;
  msg << "decision.hh:" << lineno << ": unresolved event: " << eventstr;
  throw std::domain_error(msg.str());
}  // unresolved_event
}  // namespace

}  // namespace nut

#pragma clang diagnostic pop

#endif  // include guard

// End of file
