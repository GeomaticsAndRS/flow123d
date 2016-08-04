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
 * @file    fe_rt_xfem.hh
 * @brief   Definitions of enriched Raviart-Thomas finite elements.
 * @author  Pavel Exner
 */

#ifndef FE_RT0_XFEM_HH_
#define FE_RT0_XFEM_HH_

#include "fem/finite_element_enriched.hh"
#include "fem/fe_rt.hh"
#include "fem/fe_p.hh"

#include "fem/global_enrichment_func.hh"
#include "fem/fe_values.hh"
#include "quadrature/qxfem.hh"
#include "mesh/ref_element.hh"

#include "system/logger.hh"

template <unsigned int dim, unsigned int spacedim> class FE_RT0_XFEM;

/**
 * @brief Raviart-Thomas element of order 0.
 *
 * The lowest order Raviart-Thomas finite element with linear basis functions
 * and continuous normal components across element sides.
 */
template <unsigned int dim, unsigned int spacedim>
class FE_RT0_XFEM : public FiniteElementEnriched<dim,spacedim>
{
    using FiniteElement<dim,spacedim>::number_of_dofs;
    using FiniteElement<dim,spacedim>::number_of_single_dofs;
//     using FiniteElement<dim,spacedim>::number_of_pairs;
//     using FiniteElement<dim,spacedim>::number_of_triples;
//     using FiniteElement<dim,spacedim>::number_of_sextuples;
    using FiniteElement<dim,spacedim>::generalized_support_points;
    using FiniteElement<dim,spacedim>::order;
    using FiniteElement<dim,spacedim>::is_scalar_fe;
//     using FiniteElement<dim,spacedim>::node_matrix;
    
    using FiniteElementEnriched<dim,spacedim>::fe;
    using FiniteElementEnriched<dim,spacedim>::pu;
    using FiniteElementEnriched<dim,spacedim>::enr;
    using FiniteElementEnriched<dim,spacedim>::n_regular_dofs_;
    
public:

    /**
     * @brief Constructor.
     */
    FE_RT0_XFEM(FE_RT0<dim,spacedim>* fe,std::vector<GlobalEnrichmentFunc<dim,spacedim>*> enr);


    /**
     * @brief Decides which additional quantities have to be computed
     * for each cell.
     */
    UpdateFlags update_each(UpdateFlags flags);

    /**
     * @brief Computes the shape function values and gradients on the actual cell
     * and fills the FEValues structure.
     *
     * @param q Quadrature.
     * @param data The precomputed finite element data on the reference cell.
     * @param fv_data The data to be computed.
     */
    void fill_fe_values(const Quadrature<dim> &quad,
                        FEInternalData &data,
                        FEValuesData<dim,spacedim> &fv_data) override;

    /**
     * @brief Computes the shape function values and gradients on the actual cell
     * and fills the FEValues structure.
     *
     * @param q Quadrature.
     * @param data The precomputed finite element data on the reference cell.
     * @param fv_data The data to be computed.
     */
    void fill_fe_values(const Quadrature<dim> &quad,
                        FEValuesData<dim,spacedim> &fv_data);
    
    /**
     * @brief Destructor.
     */
    ~FE_RT0_XFEM() {};

};






template <unsigned int dim, unsigned int spacedim>
FE_RT0_XFEM<dim,spacedim>::FE_RT0_XFEM(FE_RT0<dim,spacedim>* fe,
                                       std::vector<GlobalEnrichmentFunc<dim,spacedim>*> enr)
: FiniteElementEnriched<dim,spacedim>(fe,enr)
{
    order = 1;
    is_scalar_fe = false;
}


template <unsigned int dim, unsigned int spacedim>
inline UpdateFlags FE_RT0_XFEM<dim,spacedim>::update_each(UpdateFlags flags)
{
    UpdateFlags f = flags;

    if (flags & update_values)
        f |= update_jacobians | update_inverse_jacobians | update_volume_elements | update_quadrature_points;

    if (flags & update_gradients)
        f |= update_jacobians | update_inverse_jacobians | update_volume_elements | update_quadrature_points;

    return f;
}


template <unsigned int dim, unsigned int spacedim>
inline void FE_RT0_XFEM<dim,spacedim>::fill_fe_values(
        const Quadrature<dim> &quad,
        FEInternalData &data,
        FEValuesData<dim,spacedim> &fv_data)
{
    fill_fe_values(quad,fv_data);
}

template <unsigned int dim, unsigned int spacedim>
inline void FE_RT0_XFEM<dim,spacedim>::fill_fe_values(
        const Quadrature<dim> &quad,
        FEValuesData<dim,spacedim> &fv_data)
{
    ElementFullIter ele = *fv_data.present_cell;
    typedef typename Space<spacedim>::Point Point;
    unsigned int j;
    
    // can we suppose for this FE and element that:
    //  - jacobian (and its inverse and determinant) is constant on the element
    //  - abuse mapping to compute the normals
    
    DBGMSG("FE_RT0_XFEM fill fe_values\n");
    
    // shape values
    if (fv_data.update_flags & update_values)
    {
//         DBGMSG("normals\n");
        // compute normals - TODO: this should do mapping, but it does it for fe_side_values..
        vector<arma::vec::fixed<spacedim>> normals(RefElement<dim>::n_sides);
        for (j = 0; j < RefElement<dim>::n_sides; j++){
            normals[j] = trans(fv_data.inverse_jacobians[0])*RefElement<dim>::normal_vector(j);
            normals[j] = normals[j]/norm(normals[j],2);
//             normals[j].print(cout);
        }
        
//         DBGMSG("interpolation\n");
        // for SGFEM
        // values of enrichment function at generalized_support_points
        // here: n_regular_dofs = RefElement<dim>::n_nodes = RefElement<dim>::n_sides = generalized_support_points.size()
        vector<vector<double>> enr_dof_val(enr.size());
        for (unsigned int w=0; w<enr.size(); w++){
            enr_dof_val[w].resize(RefElement<dim>::n_sides);
            for (unsigned int i = 0; i < RefElement<dim>::n_sides; i++)
            {
                // compute barycenter of the side ( = real generalized_support_point)
                Point real_point;
                real_point.zeros();
                for (j = 0; j < RefElement<dim>::n_nodes_per_side; j++)
                    real_point = real_point + ele->node[RefElement<dim>::template interact<0,1>(i)[j]]->point();
                real_point = real_point / RefElement<dim>::n_nodes_per_side;
                
//                 enr[w]->vector(real_point).print(cout);
//                 normals[i].print(cout);
                enr_dof_val[w][i] = arma::dot(enr[w]->vector(real_point), normals[i]);
            }
        }

        
        vector<arma::vec::fixed<spacedim> > vectors(number_of_dofs);
        
        for (unsigned int q = 0; q < quad.size(); q++)
        {
//             DBGMSG("pu q[%d]\n",q);
            // compute PU
            arma::vec pu_values(RefElement<dim>::n_nodes);
            for (j=0; j<RefElement<dim>::n_nodes; j++)
                    pu_values[j] = pu.basis_value(j, quad.point(q));
            pu_values = pu.get_node_matrix() * pu_values;
            
//             DBGMSG("pu grad q[%d]\n",q);
            // compute PU grads
//             arma::mat pu_grads(RefElement<dim>::n_nodes, dim);
//             for (j=0; j<RefElement<dim>::n_nodes; j++)
//                 pu_grads.row(j) = arma::trans(pu.basis_grad(j, quad.point(q)));
//             pu_grads = pu.node_matrix * pu_grads;
            // real_pu_grad = pu_grads[i] * fv_data.inverse_jacobians[i];
            
            
//             DBGMSG("regular shape vectors q[%d]\n",q);
            //fill regular shape functions
            for (j=0; j<n_regular_dofs_; j++)
                vectors[j] = fv_data.jacobians[q] * fe->basis_vector(j,quad.point(q)) / fv_data.determinants[q];
            
            j = n_regular_dofs_;
            for (unsigned int w=0; w<enr.size(); w++)
            {
//                 DBGMSG("interpolant w[%d] q[%d]\n",w,q);
                //compute interpolant
                arma::vec::fixed<spacedim> interpolant;
                interpolant.zeros();
                for (unsigned int k=0; k < n_regular_dofs_; k++)
                    interpolant += vectors[k] * enr_dof_val[w][k];
                
//                 quad.real_point(q).print(cout);
//                 DBGMSG("enriched shape value w[%d] q[%d]\n",w,q);
                for (unsigned int k=0; k < pu.n_dofs(); k++)
                {
                    vectors[j] =  pu_values[k] * (enr[w]->vector(fv_data.points[q]) - interpolant);
//                     vectors[j] =  interpolant;//(enr[w]->vector(fv_data.points[q]) - interpolant);
//                     vectors[j] =  enr[w]->vector(fv_data.points[q]);
                    j++;
                }
            }   
                
            fv_data.shape_vectors[q] = vectors;
        }
    }

//     // shape gradients
//     if (fv_data.update_flags & update_gradients)
//     {
//         vector<arma::mat::fixed<spacedim,spacedim> > grads;
//         grads.resize(dim+1);
//         for (unsigned int q = 0; q < quad.size(); q++)
//         {
//             for (unsigned int k=0; k<dim+1; k++)
//                 grads[k] = fv_data.jacobians[q]*data.basis_grad_vectors[q][k]*fv_data.inverse_jacobians[q]/fv_data.determinants[q];
// 
//             fv_data.shape_grad_vectors[q] = grads;
//         }
//     }
}

#endif // FE_RT0_XFEM_HH_