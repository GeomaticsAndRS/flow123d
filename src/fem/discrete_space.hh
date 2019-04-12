/*!
 *
﻿ * Copyright (C) 2015 Technical University of Liberec.  All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 3 as published by the
 * Free Software Foundation. (http://www.gnu.org/licenses/gpl-3.0.en.html)
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * 
 * @file    discrete_space.hh
 * @brief   Declaration of class which provides the finite element for every mesh cell.
 * @author  Jan Stebel
 */

#ifndef DISCRETE_SPACE_HH_
#define DISCRETE_SPACE_HH_

#include "mesh/accessors.hh"
#include "tools/mixed.hh"
#include "mesh/duplicate_nodes.h"
#include "fem/finite_element.hh"
template<unsigned int dim> class FiniteElement;
template<Dim dim>
using FEPtr = std::shared_ptr<FiniteElement<dim>>;
class Mesh;


/**
 * Abstract class for definition of finite element functions on the mesh.
 * This should include
 * - simple FE spaces using the same finite element on
 *   all mesh elements with the same dimension,
 * - p-refined spaces using variable FE order,
 * - XFEM with arbitrary shape functions on every element.
 * 
 */
class DiscreteSpace {
public:
    
  /// Number of dofs associated to node. @p nid is the node index in the mesh tree.
  virtual unsigned int n_node_dofs(unsigned int nid) const = 0;

  /// Number of dofs associated to edge.
  virtual unsigned int n_edge_dofs(const Edge &edge) const = 0;
  
  /// Number of dofs associated to element (not shared by adjacent elements).
  virtual unsigned int n_elem_dofs(const ElementAccessor<3> &cell) const = 0;
  
  /// Number of dofs associated to generalized n-face (node, line, triangle or tetrahedron).
  template<unsigned int dim>
  unsigned int n_face_dofs(unsigned int face_id)
  {
    ASSERT(false).error("Not implemented.");
    return 0;
  }
  
  /// Return finite element object for given element.
  template<unsigned int dim>
  FEPtr<dim> fe(const ElementAccessor<3> &) const;

  /// Destructor.
  virtual ~DiscreteSpace() {};
    

protected:
  
  /// Constructor.
  DiscreteSpace(Mesh *mesh)
  : mesh_(mesh) {}
  
  virtual FEPtr<0> fe0d(const ElementAccessor<3> &) const = 0;
  virtual FEPtr<1> fe1d(const ElementAccessor<3> &) const = 0;
  virtual FEPtr<2> fe2d(const ElementAccessor<3> &) const = 0;
  virtual FEPtr<3> fe3d(const ElementAccessor<3> &) const = 0;
  
  Mesh *mesh_;

};



/**
 * Implementation of DiscreteSpace when all elements have the same FiniteElement.
 */
class EqualOrderDiscreteSpace : public DiscreteSpace {
public:
  EqualOrderDiscreteSpace(Mesh *mesh, MixedPtr<FiniteElement> fe)
  : DiscreteSpace(mesh), fe_(fe),
    _n_elem_dofs(4, 0),
    _n_edge_dofs(4, 0),
    _n_node_dofs(4, 0)
  {
      _init_n_dofs<0>();
      _init_n_dofs<1>();
      _init_n_dofs<2>();
      _init_n_dofs<3>();
  }

  unsigned int n_elem_dofs(const ElementAccessor<3> &cell) const override
  {return _n_elem_dofs[cell.dim()];}
  
  unsigned int n_edge_dofs(const Edge &edge) const override
  {return _n_edge_dofs[edge.side(0)->dim() + 1];}
  
  unsigned int n_node_dofs(unsigned int nid) const override
  {return _n_node_dofs[mesh_->tree->node_dim()[nid]];}
  
  FEPtr<0> fe0d(const ElementAccessor<3> &cell) const override { return fe_.get<0>(); }
  FEPtr<1> fe1d(const ElementAccessor<3> &cell) const override { return fe_.get<1>(); }
  FEPtr<2> fe2d(const ElementAccessor<3> &cell) const override { return fe_.get<2>(); }
  FEPtr<3> fe3d(const ElementAccessor<3> &cell) const override { return fe_.get<3>(); }
  
  
private:
  template<Dim dim>
  void _init_n_dofs() {
      auto fe_ptr = fe_.get<dim>();
      for (unsigned int d=0; d < fe_ptr->n_dofs(); d++) {
          if (fe_ptr->dof(d).dim == 0)
              _n_elem_dofs[dim]++;
          if (fe_ptr->dof(d).dim == dim && fe_ptr->dof(d).n_face_idx == 0)
              _n_edge_dofs[dim]++;
          if (fe_ptr->dof(d).dim == 0 && fe_ptr->dof(d).n_face_idx == 0)
              _n_node_dofs[dim]++;
      }
  }

  MixedPtr<FiniteElement> fe_;
  std::vector<unsigned int> _n_elem_dofs;
  std::vector<unsigned int> _n_edge_dofs;
  std::vector<unsigned int> _n_node_dofs;
  

};







#endif /* DISCRETE_SPACE_HH_ */
