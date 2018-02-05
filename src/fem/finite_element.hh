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
 * @file    finite_element.hh
 * @brief   Abstract class for description of finite elements.
 * @author  Jan Stebel
 */

#ifndef FINITE_ELEMENT_HH_
#define FINITE_ELEMENT_HH_

#include <armadillo>
#include <map>
#include <vector>
#include <boost/assign/list_of.hpp>
#include "fem/update_flags.hh"



template<unsigned int dim, unsigned int spacedim> class FESystem;
template<unsigned int dim, unsigned int spacedim> class FESideValues;
template<unsigned int dim, unsigned int spacedim> class FEValues;
template<unsigned int dim, unsigned int spacedim> class FEValuesBase;
template<unsigned int dim, unsigned int spacedim> class FEValuesData;
template<unsigned int dim, unsigned int spacedim> class FE_P_disc;
template<unsigned int dim> class Quadrature;





/**
 * @brief Multiplicity of finite element dofs.
 *
 * Multiplicities describe groups of dofs whose order changes with
 * the configuration (e.g. the rotation or orientation) of
 * the geometrical entity relative to the actual cell.
 *
 * In each spatial dimension we accept the following dof multiplicities:
 *
 * 0) Point (1 configuration):
 *    - single dofs
 *
 * 1) Line (2 possible configurations=orientations):
 *    - single dofs
 *    - pairs
 *
 * 2) Triangle (2 orientations and 3 rotations=6 configurations):
 *    - single dofs
 *    - pairs
 *    - triples
 *    - sextuples
 *
 * 3) Tetrahedron (1 configuration, since it is always the cell):
 *    - single dofs
 */
enum DofMultiplicity {
    DOF_SINGLE = 1, DOF_PAIR = 2, DOF_TRIPLE = 3, DOF_SEXTUPLE = 6
};

const std::vector<DofMultiplicity> dof_multiplicities = boost::assign::list_of(
        DOF_SINGLE)(DOF_PAIR)(DOF_TRIPLE)(DOF_SEXTUPLE);

// Possible types are: value, normal derivative, tangential derivative, ...
enum DofType { Value = 1 };
        
class Dof {
public:
    
    Dof(unsigned int dim_, arma::vec coords_, arma::vec coefs_, DofType type_)
        : dim(dim_), coords(coords_), coefs(coefs_), type(type_) {}
    
    /// Evaulate dof for basis function of given function space.
    template<class FS> const double evaluate(const FS &function_space, unsigned int basis_idx) const;
    
    /// Association to n-face of given dimension (point, line, triangle, tetrahedron.
    unsigned int dim;
    
    /// Barycentric coordinates.
    arma::vec coords;
    
    /// Coefficients of linear combination of function value components.
    arma::vec coefs;
    
    DofType type;
};

class FunctionSpace {
public:
    
    /**
     * @brief Value of the @p i th basis function at point @p point.
     * @param basis_index  Index of the basis function.
     * @param point        Point coordinates.
     * @param comp_index   Index of component (>0 for vector-valued functions).
     */
    virtual const double basis_value(unsigned int basis_index,
                                     const arma::vec &point,
                                     unsigned int comp_index = 0
                                    ) const = 0;
    
    /**
     * @brief Gradient of the @p i th basis function at point @p point.
     * @param basis_index  Index of the basis function.
     * @param point        Point coordinates.
     * @param comp_index   Index of component (>0 for vector-valued functions).
     */
    virtual const arma::vec basis_grad(unsigned int basis_index,
                                       const arma::vec &point,
                                       unsigned int comp_index = 0
                                      ) const = 0;
    
    /// Dimension of function space (number of basis functions).
    virtual const unsigned int dim() const = 0;
    
    const unsigned int space_dim() const { return space_dim_; }
    
    const unsigned int n_components() const { return n_components_; }
    
    virtual ~FunctionSpace() {}
    
protected:
    
    /// Space dimension of function arguments (i.e. 1, 2 or 3).
    unsigned int space_dim_;
    
    /// Number of components of function values.
    unsigned int n_components_;
};


/// Types of FiniteElement: scalar, vector-valued, tensor-valued or mixed system.
enum FEType {
  FEScalar = 0,
  FEVector = 1,
  FETensor = 2,
  FEMixedSystem = 3
};

/**
 * @brief Structure for storing the precomputed finite element data.
 */
class FEInternalData
{
public:
    /**
     * @brief Precomputed values of basis functions at the quadrature points.
     */
    std::vector<arma::vec> basis_values;

    /**
     * @brief Precomputed gradients of basis functions at the quadrature points.
     */
    std::vector<arma::mat > basis_grads;


    /**
     * @brief Precomputed values of basis functions at the quadrature points.
     *
     * For vectorial finite elements.
     */
    std::vector<std::vector<arma::vec> > basis_vectors;

    /**
     * @brief Precomputed gradients of basis functions at the quadrature points.
     *
     * For vectorial finite elements:
     */
    std::vector<std::vector<arma::mat> > basis_grad_vectors;
};


/**
 * @brief Abstract class for the description of a general finite element on
 * a reference simplex in @p dim dimensions.
 *
 * Description of dofs:
 *
 * The reference cell consists of lower dimensional entities (points,
 * lines, triangles). Each dof is associated to one of these
 * entities. This means that if the entity is shared by 2 or more
 * neighbouring cells in the mesh then this dof is shared by the
 * finite elements on all of these cells. If a dof is associated
 * to the cell itself then it is not shared with neighbouring cells.
 * The ordering of nodes in the entity may not be appropriate for the
 * finite elements on the neighbouring cells, hence we need to
 * describe how the order of dofs changes with the relative
 * configuration of the entity with respect to the actual cell.
 * For this reason we define the dof multiplicity which allows to
 * group the dofs as described in \ref DofMultiplicity.
 * 
 * 
 * Dof ordering:
 * 
 * The dofs and basis functions are ordered as follows:
 * - all dofs on point 1
 * - all dofs on point 2
 * ...
 * - all dofs on line 1
 * - all dofs on line 2
 * ...
 * - all dofs on triangle 1
 * - all dofs on triangle 2
 * ...
 * - all dofs on tetrahedron
 * The ordering is important for compatibility with DOFHandler
 * and FESystem.
 * 
 * 
 * Support points:
 *
 * Sometimes it is convenient to describe the function space using
 * a basis (called the raw basis) that is different from the set of
 * shape functions for the finite element (the actual basis). For
 * this reason we define the support points which play the role of
 * nodal functionals associated to the particular dofs. To convert
 * between the two bases one can use the @p node_matrix, which is
 * constructed by the method compute_node_matrix(). In the case of
 * non-Lagrangean finite elements the dofs are not associated to
 * nodal functionals but e.g. to derivatives or averages. For that
 * reason we distinguish between the unit support points which are
 * uniquely associated to the dofs and the generalized support
 * points that are auxiliary for the calculation of the dof
 * functionals.
 *
 *
 */
template<unsigned int dim, unsigned int spacedim>
class FiniteElement {
public:
  
    /**
     * @brief Constructor.
     */
    FiniteElement();
    
    /**
     * @brief Returns the number of degrees of freedom needed by the finite
     * element.
     */
    const unsigned int n_dofs() const;

    /**
     * @brief Returns the number of single dofs/dof pairs/triples/sextuples
     * that lie on a single geometric entity of the dimension
     * @p object_dim.
     *
     * @param object_dim Dimension of the geometric entity.
     * @param multiplicity Multiplicity of dofs.
     */
    const unsigned int n_object_dofs(unsigned int object_dim,
            DofMultiplicity multiplicity);

    /**
     * @brief Calculates the value of the @p comp-th component of
     * the @p i-th raw basis function at the
     * point @p p on the reference element.
     *
     * @param i    Number of the basis function.
     * @param p    Point of evaluation.
     * @param comp Number of vector component.
     */
    virtual double basis_value(const unsigned int i,
            const arma::vec::fixed<dim> &p, const unsigned int comp = 0) const;

    /**
     * @brief Calculates the @p comp-th component of the gradient
     * of the @p i-th raw basis function at the point @p p on the
     * reference element.
     *
     * @param i    Number of the basis function.
     * @param p    Point of evaluation.
     * @param comp Number of vector component.
     */
    virtual arma::vec::fixed<dim> basis_grad(const unsigned int i,
            const arma::vec::fixed<dim> &p, const unsigned int comp = 0) const;

    /// Returns numer of components of the basis function.    
    inline unsigned int n_components() const {
      return n_components_;
    }
    
    Dof dof(unsigned int i) const {
        return dofs_[i]; 
    }
    
    /**
     * @brief Destructor.
     */
    virtual ~FiniteElement();

protected:
  
    /**
     * @brief Clears all internal structures.
     */
    void init(unsigned int n_components = 1, bool primitive = true, FEType type = FEScalar);
    
    /**
     * @brief Initialize vectors with information about components of basis functions.
     */
    void setup_components();
    
    /**
     * @brief Calculates the data on the reference cell.
     *
     * @param q Quadrature rule.
     * @param flags Update flags.
     */
    virtual FEInternalData *initialize(const Quadrature<dim> &q);

    /**
     * @brief Decides which additional quantities have to be computed
     * for each cell.
     *
     * @param flags Computed update flags.
     */
    virtual UpdateFlags update_each(UpdateFlags flags);

    /**
     * @brief Computes the shape function values and gradients on the actual cell
     * and fills the FEValues structure.
     *
     * @param q Quadrature rule.
     * @param data Precomputed finite element data.
     * @param fv_data Data to be computed.
     */
    virtual void fill_fe_values(const Quadrature<dim> &q,
            FEInternalData &data,
            FEValuesData<dim,spacedim> &fv_data);

    /**
     * @brief Initializes the @p node_matrix for computing the coefficients
     * of the raw basis functions from values at support points.
     *
     * The method is implemented for the case of Langrangean finite
     * element. In other cases it may be reimplemented.
     */
    virtual void compute_node_matrix();
    
    /**
     * @brief Indicates whether the basis functions have one or more
     * nonzero components (scalar FE spaces are always primitive).
     */
    inline const bool is_primitive() const {
        return is_primitive_;
    }
    
    /**
     * @brief Returns the component index for vector valued finite elements.
     * @param sys_idx Index of shape function.
     */
    unsigned int system_to_component_index(unsigned sys_idx) const {
      return component_indices_[sys_idx];
    }
    
    /**
     * @brief Returns the mask of nonzero components for given basis function.
     * @param sys_idx Index of basis function.
     */
    const std::vector<bool> &get_nonzero_components(unsigned int sys_idx) const {
      return nonzero_components_[sys_idx];
    }

    /**
     * @brief Total number of degrees of freedom at one finite element.
     */
    unsigned int number_of_dofs;

    /**
     * @brief Number of single dofs at one geometrical entity of the given
     * dimension (point, line, triangle, tetrahedron).
     */
    unsigned int number_of_single_dofs[dim + 1];

    /**
     * @brief Number of pairs of dofs at one geometrical entity of the given
     * dimension (applicable to lines and triangles).
     */
    unsigned int number_of_pairs[dim + 1];

    /**
     * @brief Number of triples of dofs associated to one triangle.
     */
    unsigned int number_of_triples[dim + 1];

    /**
     * @brief Number of sextuples of dofs associated to one triangle.
     */
    unsigned int number_of_sextuples[dim + 1];
    
    /// Type of FiniteElement.
    FEType type_;

    /**
     * @brief Primitive FE is using componentwise shape functions,
     * i.e. only one component is nonzero for each shape function.
     */
    bool is_primitive_;
    
    /// Number of components of shape functions.
    unsigned int n_components_;
    
    /// Indices of nonzero components of shape functions (for primitive FE).
    std::vector<unsigned int> component_indices_;
    
    std::vector<std::vector<bool> > nonzero_components_;

    /**
     * @brief Matrix that determines the coefficients of the raw basis
     * functions from the values at the support points.
     */
    arma::mat node_matrix;

    /// Function space defining the FE.
    FunctionSpace *function_space_;
    
    /// Set of degrees of freedom (functionals) defining the FE.
    std::vector<Dof> dofs_;
    
    
    friend class FESystem<dim,spacedim>;
    friend class FEValuesBase<dim,spacedim>;
    friend class FEValues<dim,spacedim>;
    friend class FESideValues<dim,spacedim>;
    friend class FE_P_disc<dim,spacedim>;
};




#endif /* FINITE_ELEMENT_HH_ */
