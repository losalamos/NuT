// Boundary.hh
// T. M. Kelley
// Oct 17, 2019
// (c) Copyright 2019 Triad National Security, all rights reserved

#pragma once

#include "types.hh"
#include <unordered_map>

namespace nut {

/**\brief Store/retrieve boundary conditions */
template <class Face_Index_T>
struct Boundary_Cond {
  using face_t = Face_Index_T;
  using desc_t = bdy_types::descriptor;
  using bc_map_t = std::unordered_map<Face_Index_T, desc_t>;

  /**\brief Get the boundary type associated with Face f.
   *
   * If no boundary is associated with the Face f, the default boundary type
   * will be associated with f, so don't ask for this if you don't want
   * to waste time on useless faces. */
  desc_t get_boundary_type(face_t f) const
  {
    // Require(is_known_boundary(f));
    return m_bcs.at(f);
  }

  bool is_known_boundary(face_t f) const { return (m_bcs.count(f) != 0); }

  void set_boundary_type(face_t f, desc_t d) { m_bcs[f] = d; }

  bc_map_t m_bcs;
};

template <typename Mesh_1D>
Boundary_Cond<typename Mesh_1D::Face>
make_vacuum_boundary_1D(Mesh_1D const & mesh)
{
  using face_t = typename Mesh_1D::Face;
  Boundary_Cond<face_t> bcs;
  face_t low{0u};
  bcs.set_boundary_type(low, bdy_types::REFLECTIVE);
  face_t high{mesh.num_cells() + 1};
  bcs.set_boundary_type(high, bdy_types::VACUUM);
  return bcs;
}  // make_vacuum_boundary(Mesh_1D)

template <typename Mesh_Cartesian_3D>
Boundary_Cond<typename Mesh_Cartesian_3D::Face>
make_vacuum_boundary_3D(Mesh_Cartesian_3D const & mesh)
{
  using index_t = typename Mesh_Cartesian_3D::index_t;
  index_t const nx = mesh.get_num_x();
  using face_t = typename Mesh_Cartesian_3D::Face;
  Boundary_Cond<face_t> bcs;

  face_t low{0u};
  bcs.set_boundary_type(low, bdy_types::REFLECTIVE);
  face_t high{mesh.num_cells()};
  bcs.set_boundary_type(high, bdy_types::VACUUM);

  throw(std::logic_error("vacuum BCs not implemented yet!"));
  return bcs;
}  // make_vacuum_boundary(Mesh_3D)

}  // namespace nut

// End of file