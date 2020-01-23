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
 * @file    fe_values.hh
 * @brief   Class FEValues calculates finite element data on the actual
 *          cells such as shape function values, gradients, Jacobian of
 *          the mapping from the reference cell etc.
 * @author  Jan Stebel
 */

#ifndef FE_VALUES_HH_
#define FE_VALUES_HH_

#include <string.h>                           // for memcpy
#include <algorithm>                          // for swap
#include <new>                                // for operator new[]
#include <string>                             // for operator<<
#include <vector>                             // for vector
#include "fem/element_values.hh"              // for ElementValues
#include "fem/fe_values_views.hh"             // for FEValuesViews
#include "mesh/ref_element.hh"                // for RefElement
#include "mesh/accessors.hh"
#include "fem/update_flags.hh"                // for UpdateFlags
#include "tools/mixed.hh"
#include "quadrature/quadrature_lib.hh"

class Quadrature;
template<unsigned int dim> class FiniteElement;












/**
 * @brief Abstract base class with certain methods independent of the template parameter @p dim.
 */
template<unsigned int spacedim = 3>
class FEValuesSpaceBase
{
public:
    virtual ~FEValuesSpaceBase() {}
    /**
     * @brief Return the value of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    virtual double shape_value(const unsigned int function_no, const unsigned int point_no) = 0;

    /**
     * @brief Return the gradient of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    virtual arma::vec::fixed<spacedim> shape_grad(const unsigned int function_no, const unsigned int point_no) = 0;

    /**
     * @brief Return the product of Jacobian determinant and the quadrature
     * weight at given quadrature point.
     *
     * @param point_no Number of the quadrature point.
     */
    virtual double JxW(const unsigned int point_no) = 0;

    /**
     * @brief Returns the normal vector to a side at given quadrature point.
     *
     * @param point_no Number of the quadrature point.
     */
    virtual arma::vec::fixed<spacedim> normal_vector(unsigned int point_no) = 0;

    /**
     * @brief Returns the number of shape functions.
     */
    virtual unsigned int n_dofs() = 0;

};

/**
 * @brief Base class for FEValues and FESideValues
 */
template<unsigned int dim, unsigned int spacedim = 3>
class FEValuesBase : public FEValuesSpaceBase<spacedim>
{
private:
  
  // internal structure that stores all possible views
  // for scalar and vector-valued components of the FE
  struct ViewsCache {
    vector<FEValuesViews::Scalar<dim,spacedim> > scalars;
    vector<FEValuesViews::Vector<dim,spacedim> > vectors;
    vector<FEValuesViews::Tensor<dim,spacedim> > tensors;
    
    void initialize(FEValuesBase &fv);
  };
  
public:

    /**
     * @brief Default constructor
     */
    FEValuesBase();


    /**
     * Correct deallocation of objects created by 'initialize' methods.
     */
    virtual ~FEValuesBase();


    /**
     * @brief Allocates space for computed data.
     *
     * @param _quadrature The quadrature rule.
     * @param _fe The finite element.
     * @param flags The update flags.
     */
    void allocate(unsigned int n_points,
            FiniteElement<dim> &_fe,
            UpdateFlags flags);
    
    /**
     * @brief Determine quantities to be recomputed on each cell.
     *
     * @param flags Flags that indicate what has to be recomputed.
     */
    UpdateFlags update_each(UpdateFlags flags);

    /**
     * @brief Return the value of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    double shape_value(const unsigned int function_no, const unsigned int point_no) override;


    /**
     * @brief Return the gradient of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    arma::vec::fixed<spacedim> shape_grad(const unsigned int function_no, const unsigned int point_no) override;

    /**
     * @brief Return the value of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * For vectorial finite elements.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    double shape_value_component(const unsigned int function_no, 
                                        const unsigned int point_no, 
                                        const unsigned int comp) const;

    /**
     * @brief Return the gradient of the @p function_no-th shape function at
     * the @p point_no-th quadrature point.
     *
     * For vectorial finite elements.
     *
     * @param function_no Number of the shape function.
     * @param point_no Number of the quadrature point.
     */
    arma::vec::fixed<spacedim> shape_grad_component(const unsigned int function_no,
                                                           const unsigned int point_no,
                                                           const unsigned int comp) const;

    /**
     * @brief Return the relative volume change of the cell (Jacobian determinant).
     *
     * If dim==spacedim then the sign may be negative, otherwise the
     * result is a positive number.
     *
     * @param point_no Number of the quadrature point.
     */
    inline double determinant(const unsigned int point_no)
    {
        ASSERT_LT_DBG(point_no, n_points_);
        return elm_values->determinant(point_no);
    }

    /**
     * @brief Return the product of Jacobian determinant and the quadrature
     * weight at given quadrature point.
     *
     * @param point_no Number of the quadrature point.
     */
    inline double JxW(const unsigned int point_no) override
    {
        ASSERT_LT_DBG(point_no, n_points_);
        // TODO: This is temporary solution to distinguish JxW on element and side_JxW on side.
        // In future we should call the appropriate method in elm_values.
        return (elm_values->cell().is_valid()) ? elm_values->JxW(point_no) : elm_values->side_JxW(point_no);
    }

    /**
     * @brief Return coordinates of the quadrature point in the actual cell system.
     *
     * @param point_no Number of the quadrature point.
     */
    inline arma::vec::fixed<spacedim> point(const unsigned int point_no)
    {
        ASSERT_LT_DBG(point_no, n_points_);
        return elm_values->point(point_no);
    }

    /**
	 * @brief Return coordinates of all quadrature points in the actual cell system.
	 *
	 */
	inline const Armor::array &point_list() const
	{
	    return elm_values->point_list();
	}


    /**
     * @brief Returns the normal vector to a side at given quadrature point.
     *
     * @param point_no Number of the quadrature point.
     */
	inline arma::vec::fixed<spacedim> normal_vector(unsigned int point_no) override
	{
        ASSERT_LT_DBG(point_no, n_points_);
	    return elm_values->normal_vector(point_no);
	}
	
	/**
     * @brief Accessor to scalar values of multicomponent FE.
     * @param i Index of scalar component.
     */
	const FEValuesViews::Scalar<dim,spacedim> &scalar_view(unsigned int i) const
	{
      ASSERT_LT_DBG(i, views_cache_.scalars.size());
      return views_cache_.scalars[i];
    }
    
    /**
     * @brief Accessor to vector values of multicomponent FE.
     * @param i Index of first vector component.
     */
    const FEValuesViews::Vector<dim,spacedim> &vector_view(unsigned int i) const
    {
      ASSERT_LT_DBG(i, views_cache_.vectors.size());
      return views_cache_.vectors[i];
    }
    
    /**
     * @brief Accessor to tensor values of multicomponent FE.
     * @param i Index of first tensor component.
     */
    const FEValuesViews::Tensor<dim,spacedim> &tensor_view(unsigned int i) const
    {
      ASSERT_LT_DBG(i, views_cache_.tensors.size());
      return views_cache_.tensors[i];
    }

    /**
     * @brief Returns the number of quadrature points.
     */
    inline unsigned int n_points()
    { return n_points_; }

    /**
     * @brief Returns the number of shape functions.
     */
    inline unsigned int n_dofs() override
    {
        return fe->n_dofs();
    }


    /**
     * @brief Returns the finite element in use.
     */
    inline FiniteElement<dim> * get_fe() const
    {
        return fe;
    }
    

protected:

    /// Structure for storing the precomputed finite element data.
    class FEInternalData
    {
    public:
        
        FEInternalData(unsigned int np, unsigned int nd);
        
        /**
         * Create a new instance of FEInternalData for a FESystem component or subvector.
         */
        FEInternalData(const FEInternalData &fe_system_data,
                    const std::vector<unsigned int> &dof_indices,
                    unsigned int first_component_idx,
                    unsigned int ncomponents = 1
                    );
        
        /**
         * @brief Precomputed values of basis functions at the quadrature points.
         *
         * Dimensions:   (no. of quadrature points)
         *             x (no. of dofs)
         *             x (no. of components in ref. cell)
         */
        std::vector<std::vector<arma::vec> > ref_shape_values;

        /**
         * @brief Precomputed gradients of basis functions at the quadrature points.
         *
         * Dimensions:   (no. of quadrature points)
         *             x (no. of dofs)
         *             x ((dim of. ref. cell)x(no. of components in ref. cell))
         */
        std::vector<std::vector<arma::mat> > ref_shape_grads;
        
        /// Number of quadrature points.
        unsigned int n_points;
        
        /// Number of dofs (shape functions).
        unsigned int n_dofs;
    };




    
    /// Precompute finite element data on reference element.
    FEInternalData *init_fe_data(const Quadrature &q);
    
    /**
     * @brief Computes the shape function values and gradients on the actual cell
     * and fills the FEValues structure.
     *
     * @param fe_data Precomputed finite element data.
     */
    void fill_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for scalar FE.
    void fill_scalar_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for vectorial FE.
    void fill_vec_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for vectorial FE.
    void fill_vec_contravariant_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for Raviart-Thomas FE.
    void fill_vec_piola_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for tensorial FE.
    void fill_tensor_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    
    /// Compute shape functions and gradients on the actual cell for mixed system of FE.
    void fill_system_data(const ElementValues<spacedim> &elm_values, const FEInternalData &fe_data);
    

    /// Number of integration points.
    unsigned int n_points_;

    /// The used finite element.
    FiniteElement<dim> *fe;
    
    /// Shape functions evaluated at the quadrature points.
    std::vector<std::vector<double> > shape_values;

    /// Gradients of shape functions evaluated at the quadrature points.
    /// Each row of the matrix contains the gradient of one shape function.
    std::vector<std::vector<arma::vec::fixed<spacedim> > > shape_gradients;

    /// Flags that indicate which finite element quantities are to be computed.
    UpdateFlags update_flags;

    /// Auxiliary object for calculation of element-dependent data.
    ElementValues<spacedim> *elm_values;
    
    /// Vector of FEValues for sub-elements of FESystem.
    std::vector<std::shared_ptr<FEValuesBase<dim,spacedim> > > fe_values_vec;
    
    /// Number of components of the FE.
    unsigned int n_components_;
    
    /// Auxiliary storage of FEValuesViews accessors.
    ViewsCache views_cache_;
};




/**
 * @brief Calculates finite element data on the actual cell.
 *
 * FEValues takes care of the calculation of finite element data on
 * the actual cell such as values of shape functions at quadrature
 * points, gradients of shape functions, Jacobians of the mapping
 * from the reference cell etc.
 * @param dim      Dimension of the reference cell.
 * @param spacedim Dimension of the Euclidean space where the actual
 *                 cell lives.
 */
template<unsigned int dim, unsigned int spacedim = 3>
class FEValues : public FEValuesBase<dim,spacedim>
{
public:

    /// Default invalid constructor.
	FEValues() : fe_data(nullptr) {}

	/**
	 * @brief Constructor.
	 *
	 * Initializes structures and calculates
         * cell-independent data.
	 *
	 * @param _quadrature The quadrature rule.
	 * @param _fe The finite element.
	 * @param _flags The update flags.
	 */
    FEValues(Quadrature &_quadrature,
             FiniteElement<dim> &_fe,
             UpdateFlags _flags);
    
    ~FEValues();

    /**
     * @brief Update cell-dependent data (gradients, Jacobians etc.)
     *
     * @param cell The actual cell.
     */
    void reinit(const ElementAccessor<spacedim> &cell);
    
    
    
private:
    
    void fill_fe_values();
    
    /// Precomputed finite element data.
    typename FEValuesBase<dim,spacedim>::FEInternalData *fe_data;
    

};


MixedPtr<FEValues> mixed_fe_values(
        QGauss::array &quadrature,
        MixedPtr<FiniteElement> fe,
        UpdateFlags flags);


/**
 * @brief Calculates finite element data on a side.
 *
 * FESideValues takes care of the calculation of finite element data on
 * a side of the actual cell such as values of shape functions at quadrature
 * points, gradients of shape functions, Jacobians of the mapping
 * from the reference cell etc.
 * @param dim      Dimension of the reference cell.
 * @param spacedim Dimension of the Euclidean space where the actual
 *                 cell lives.
 */
template<unsigned int dim, unsigned int spacedim>
class FESideValues : public FEValuesBase<dim,spacedim>
{

public:

    /**
     * @brief Constructor.
     *
     * Initializes structures and calculates
     * cell-independent data.
     *
     * @param _sub_quadrature The quadrature rule on the side.
     * @param _fe The finite element.
     * @param flags The update flags.
     */
    FESideValues(Quadrature &_sub_quadrature,
             FiniteElement<dim> &_fe,
             UpdateFlags flags);

    /// Destructor.
    virtual ~FESideValues();

    /**
	 * @brief Update cell-dependent FE data (values, gradients).
	 *
	 * @param cell_side Accessor to cell side.
	 */
    void reinit(const Side &cell_side);


private:
    
    /// Calculates the mapping data on a side of a cell.
    void fill_fe_side_values();
    
    /// Internal data (shape functions on reference element) for all sides and permuted quadrature points.
    typename FEValuesBase<dim,spacedim>::FEInternalData *side_fe_data[RefElement<dim>::n_sides][RefElement<dim>::n_side_permutations];
    
};





#endif /* FE_VALUES_HH_ */
