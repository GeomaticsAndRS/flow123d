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
 * @file    generic_assembly.hh
 * @brief
 */

#ifndef GENERIC_ASSEMBLY_HH_
#define GENERIC_ASSEMBLY_HH_

#include "quadrature/quadrature_lib.hh"
#include "fields/eval_subset.hh"
#include "fields/eval_points.hh"
#include "fields/field_value_cache.hh"



/// Allow set mask of active integrals.
enum ActiveIntegrals {
    none     =      0,
    bulk     = 0x0001,
    edge     = 0x0002,
    coupling = 0x0004,
    boundary = 0x0008
};


/// Set of all used integral necessary in assemblation
struct AssemblyIntegrals {
    std::array<std::shared_ptr<BulkIntegral>, 3> bulk_;          ///< Bulk integrals of elements of dimensions 1, 2, 3
    std::array<std::shared_ptr<BulkIntegral>, 4> center_;        ///< Bulk integrals represents center of elements of dimensions 0, 1, 2, 3
    std::array<std::shared_ptr<EdgeIntegral>, 3> edge_;          ///< Edge integrals between elements of dimensions 1, 2, 3
    std::array<std::shared_ptr<CouplingIntegral>, 2> coupling_;  ///< Coupling integrals between elements of dimensions 1-2, 2-3
    std::array<std::shared_ptr<BoundaryIntegral>, 3> boundary_;  ///< Boundary integrals betwwen elements of dimensions 1, 2, 3 and boundaries
};



/**
 * @brief Generic class of assemblation.
 *
 * Class
 *  - holds assemblation structures (EvalPoints, Integral objects, Integral data table).
 *  - associates assemblation objects specified by dimension
 *  - provides general assemble method
 *  - provides methods that allow construction of element patches
 */
template < template<IntDim...> class DimAssembly>
class GenericAssembly
{
private:
    struct BulkIntegralData {
        BulkIntegralData() {}

        DHCellAccessor cell;
        unsigned int subset_index;
    };

    struct EdgeIntegralData {
    	EdgeIntegralData()
    	: edge_side_range(make_iter<DHEdgeSide, DHCellSide>( DHEdgeSide() ), make_iter<DHEdgeSide, DHCellSide>( DHEdgeSide() )) {}

    	RangeConvert<DHEdgeSide, DHCellSide> edge_side_range;
        unsigned int subset_index;
	};

    struct CouplingIntegralData {
       	CouplingIntegralData() {}

        DHCellAccessor cell;
	    unsigned int bulk_subset_index;
        DHCellSide side;
	    unsigned int side_subset_index;
    };

    struct BoundaryIntegralData {
    	BoundaryIntegralData() {}

    	// We don't need hold ElementAccessor of boundary element, side.cond().element_accessor() provides it.
	    unsigned int bdr_subset_index; // index of subset on boundary element
	    DHCellSide side;
	    unsigned int side_subset_index;
	};

    /**
     * Temporary struct holds data od boundary element.
     *
     * It will be merge to BulkIntegralData after making implementation of DHCellAccessor on boundary elements.
     */
    struct BdrElementIntegralData {
    	BdrElementIntegralData() {}

        ElementAccessor<3> elm;
        unsigned int subset_index;
    };

public:

    /// Constructor
    GenericAssembly( typename DimAssembly<1>::EqDataDG *eq_data, int active_integrals )
    : multidim_assembly_(eq_data),
      active_integrals_(active_integrals), integrals_size_({0, 0, 0, 0, 0, 0})
    {
        eval_points_ = std::make_shared<EvalPoints>();
        // first step - create integrals, then - initialize cache
        Quadrature *quad_center_0d = new QGauss(0, 1);
        integrals_.center_[0] = eval_points_->add_bulk<0>(*quad_center_0d);
        multidim_assembly_[1_d]->create_integrals(eval_points_, integrals_, active_integrals_);
        multidim_assembly_[2_d]->create_integrals(eval_points_, integrals_, active_integrals_);
        multidim_assembly_[3_d]->create_integrals(eval_points_, integrals_, active_integrals_);
        element_cache_map_.init(eval_points_);
    }

    inline MixedPtr<DimAssembly, 1> multidim_assembly() const {
        return multidim_assembly_;
    }

    inline std::shared_ptr<EvalPoints> eval_points() const {
        return eval_points_;
    }

	/**
	 * @brief General assemble methods.
	 *
	 * Loops through local cells and calls assemble methods of assembly
	 * object of each cells over space dimension.
	 */
    void assemble(std::shared_ptr<DOFHandlerMultiDim> dh) {
        unsigned int i;
        multidim_assembly_[1_d]->reallocate_cache(element_cache_map_);
        multidim_assembly_[1_d]->begin();
        for (auto cell : dh->local_range() )
        {
            this->add_integrals_of_computing_step(cell);
            element_cache_map_(cell);

            if ( cell.is_own() && (active_integrals_ & ActiveIntegrals::bulk) ) {
                START_TIMER("assemble_volume_integrals");
                for (i=0; i<integrals_size_[0]; ++i) { // volume integral
                    switch (bulk_integral_data_[i].cell.dim()) {
                    case 1:
                        multidim_assembly_[1_d]->assemble_volume_integrals(bulk_integral_data_[i].cell);
                        break;
                    case 2:
                        multidim_assembly_[2_d]->assemble_volume_integrals(bulk_integral_data_[i].cell);
                        break;
                    case 3:
                        multidim_assembly_[3_d]->assemble_volume_integrals(bulk_integral_data_[i].cell);
                        break;
                    }
                }
                END_TIMER("assemble_volume_integrals");
            }

            if ( cell.is_own() && (active_integrals_ & ActiveIntegrals::boundary) ) {
                START_TIMER("assemble_fluxes_boundary");
                for (i=0; i<integrals_size_[3]; ++i) { // boundary integral
                    switch (boundary_integral_data_[i].side.dim()) {
                    case 1:
                        multidim_assembly_[1_d]->assemble_fluxes_boundary(boundary_integral_data_[i].side);
                        break;
                    case 2:
                        multidim_assembly_[2_d]->assemble_fluxes_boundary(boundary_integral_data_[i].side);
                        break;
                    case 3:
                        multidim_assembly_[3_d]->assemble_fluxes_boundary(boundary_integral_data_[i].side);
                        break;
                    }
                }
                END_TIMER("assemble_fluxes_boundary");
            }

            if (active_integrals_ & ActiveIntegrals::edge) {
                START_TIMER("assemble_fluxes_elem_elem");
                for (i=0; i<integrals_size_[1]; ++i) { // edge integral
                    switch (edge_integral_data_[i].edge_side_range.begin()->dim()) {
                    case 1:
                        multidim_assembly_[1_d]->assemble_fluxes_element_element(edge_integral_data_[i].edge_side_range);
                        break;
                    case 2:
                        multidim_assembly_[2_d]->assemble_fluxes_element_element(edge_integral_data_[i].edge_side_range);
                        break;
                    case 3:
                        multidim_assembly_[3_d]->assemble_fluxes_element_element(edge_integral_data_[i].edge_side_range);
                        break;
                    }
                }
                END_TIMER("assemble_fluxes_elem_elem");
            }

            if (active_integrals_ & ActiveIntegrals::coupling) {
                START_TIMER("assemble_fluxes_elem_side");
                for (i=0; i<integrals_size_[2]; ++i) { // coupling integral
                    switch (coupling_integral_data_[i].side.dim()) {
                    case 2:
                        multidim_assembly_[2_d]->assemble_fluxes_element_side(coupling_integral_data_[i].cell, coupling_integral_data_[i].side);
                        break;
                    case 3:
                        multidim_assembly_[3_d]->assemble_fluxes_element_side(coupling_integral_data_[i].cell, coupling_integral_data_[i].side);
                        break;
                    }
                }
                END_TIMER("assemble_fluxes_elem_side");
            }
        }
        multidim_assembly_[1_d]->end();
    }

    /// Return ElementCacheMap
    inline const ElementCacheMap &cache_map() const {
        return element_cache_map_;
    }

    /// Return BulkIntegral of appropriate dimension
    inline std::shared_ptr<BulkIntegral> bulk_integral(unsigned int dim) const {
        ASSERT_DBG( (dim>0) && (dim<=3) )(dim).error("Invalid dimension, must be 1, 2 or 3!\n");
	    return integrals_.bulk_[dim-1];
    }

    /// Return CenterIntegral of appropriate dimension
    inline std::shared_ptr<BulkIntegral> center_integral(unsigned int dim) const {
        ASSERT_DBG( dim<=3 )(dim).error("Invalid dimension, must be 0, 1, 2 or 3!\n");
        return integrals_.center_[dim];
    }

    /// Return EdgeIntegral of appropriate dimension
    inline std::shared_ptr<EdgeIntegral> edge_integral(unsigned int dim) const {
        ASSERT_DBG( (dim>0) && (dim<=3) )(dim).error("Invalid dimension, must be 1, 2 or 3!\n");
	    return integrals_.edge_[dim-1];
    }

    /// Return CouplingIntegral between dimensions dim-1 and dim
    inline std::shared_ptr<CouplingIntegral> coupling_integral(unsigned int dim) const {
        ASSERT_DBG( (dim>1) && (dim<=3) )(dim).error("Invalid dimension, must be 2 or 3!\n");
	    return integrals_.coupling_[dim-2];
    }

    /// Return BoundaryIntegral of appropriate dimension
    inline std::shared_ptr<BoundaryIntegral> boundary_integral(unsigned int dim) const {
        ASSERT_DBG( (dim>0) && (dim<=3) )(dim).error("Invalid dimension, must be 1, 2 or 3!\n");
	    return integrals_.boundary_[dim-1];
    }

private:
    /// Mark eval points in table of Element cache map.
    void insert_eval_points_from_integral_data() {
        for (unsigned int i=0; i<integrals_size_[0]; ++i) {
            // add data to cache if there is free space, else return
        	unsigned int data_size = eval_points_->subset_size( bulk_integral_data_[i].cell.dim(), bulk_integral_data_[i].subset_index );
        	element_cache_map_.mark_used_eval_points(bulk_integral_data_[i].cell, bulk_integral_data_[i].subset_index, data_size);
        }

        for (unsigned int i=0; i<integrals_size_[1]; ++i) {
            // add data to cache if there is free space, else return
        	for (DHCellSide edge_side : edge_integral_data_[i].edge_side_range) {
        	    unsigned int side_dim = edge_side.dim();
                unsigned int data_size = eval_points_->subset_size( side_dim, edge_integral_data_[i].subset_index ) / (side_dim+1);
                unsigned int start_point = data_size * edge_side.side_idx();
                element_cache_map_.mark_used_eval_points(edge_side.cell(), edge_integral_data_[i].subset_index, data_size, start_point);
        	}
        }

        for (unsigned int i=0; i<integrals_size_[2]; ++i) {
            // add data to cache if there is free space, else return
            unsigned int bulk_data_size = eval_points_->subset_size( coupling_integral_data_[i].cell.dim(), coupling_integral_data_[i].bulk_subset_index );
            element_cache_map_.mark_used_eval_points(coupling_integral_data_[i].cell, coupling_integral_data_[i].bulk_subset_index, bulk_data_size);

            unsigned int side_dim = coupling_integral_data_[i].side.dim();
            unsigned int side_data_size = eval_points_->subset_size( side_dim, coupling_integral_data_[i].side_subset_index ) / (side_dim+1);
            unsigned int start_point = side_data_size * coupling_integral_data_[i].side.side_idx();
            element_cache_map_.mark_used_eval_points(coupling_integral_data_[i].side.cell(), coupling_integral_data_[i].side_subset_index, side_data_size, start_point);
        }

        for (unsigned int i=0; i<integrals_size_[3]; ++i) {
            // add data to cache if there is free space, else return
            unsigned int bdr_data_size = eval_points_->subset_size( boundary_integral_data_[i].side.cond().element_accessor().dim(), boundary_integral_data_[i].bdr_subset_index );
            element_cache_map_.mark_used_eval_points(boundary_integral_data_[i].side.cond().element_accessor(), boundary_integral_data_[i].bdr_subset_index, bdr_data_size);

            unsigned int side_dim = boundary_integral_data_[i].side.dim();
            unsigned int data_size = eval_points_->subset_size( side_dim, boundary_integral_data_[i].side_subset_index ) / (side_dim+1);
            unsigned int start_point = data_size * boundary_integral_data_[i].side.side_idx();
            element_cache_map_.mark_used_eval_points(boundary_integral_data_[i].side.cell(), boundary_integral_data_[i].side_subset_index, data_size, start_point);
        }

        for (unsigned int i=0; i<integrals_size_[4]; ++i) {
            // add data to cache if there is free space, else return
        	unsigned int data_size = eval_points_->subset_size( center_integral_data_[i].cell.dim(), center_integral_data_[i].subset_index );
        	element_cache_map_.mark_used_eval_points(center_integral_data_[i].cell, center_integral_data_[i].subset_index, data_size);
        }

        for (unsigned int i=0; i<integrals_size_[5]; ++i) {
            // add data to cache if there is free space, else return
        	unsigned int data_size = eval_points_->subset_size( bdr_elem_integral_data_[i].elm.dim(), bdr_elem_integral_data_[i].subset_index );
        	element_cache_map_.mark_used_eval_points(bdr_elem_integral_data_[i].elm, bdr_elem_integral_data_[i].subset_index, data_size);
        }
    }

    /**
     * Add data of integrals to appropriate structure and register elements to ElementCacheMap.
     *
     * Types of used integrals must be set in data member \p active_integrals_.
     */
    void add_integrals_of_computing_step(DHCellAccessor cell) {
        for (unsigned int i=0; i<6; i++) integrals_size_[i] = 0; // clean integral data from previous step
        element_cache_map_.start_elements_update();

        // generic_assembly.check_integral_data();
        this->add_center_integral(cell);
        element_cache_map_.add(cell);

        if (active_integrals_ & ActiveIntegrals::bulk)
    	    if (cell.is_own()) { // Not ghost
                this->add_volume_integral(cell);
    	    }

        for( DHCellSide cell_side : cell.side_range() ) {
            if (active_integrals_ & ActiveIntegrals::boundary)
                if (cell.is_own()) // Not ghost
                    if ( (cell_side.side().edge().n_sides() == 1) && (cell_side.side().is_boundary()) ) {
                        this->add_boundary_integral(cell_side);
                        element_cache_map_.add(cell_side);
                        auto bdr_elm_acc = cell_side.cond().element_accessor();
                        this->add_bdr_elem_integral( bdr_elm_acc ); // TODO need DHCell constructed from boundary element
                        element_cache_map_.add(bdr_elm_acc);
                        continue;
                    }
            if (active_integrals_ & ActiveIntegrals::edge)
                if ( (cell_side.n_edge_sides() >= 2) && (cell_side.edge_sides().begin()->element().idx() == cell.elm_idx())) {
                    this->add_edge_integral(cell_side.edge_sides());
                	for( DHCellSide edge_side : cell_side.edge_sides() ) {
                	    if (cell_side.elem_idx() != edge_side.elem_idx())
                	        this->add_center_integral(edge_side.cell());
                		element_cache_map_.add(edge_side);
                    }
                }
        }

        if (active_integrals_ & ActiveIntegrals::coupling)
            for( DHCellSide neighb_side : cell.neighb_sides() ) { // cell -> elm lower dim, neighb_side -> elm higher dim
                if (cell.dim() != neighb_side.dim()-1) continue;
                this->add_coupling_integral(cell, neighb_side);
                this->add_center_integral(neighb_side.cell());
                element_cache_map_.add(cell);
                element_cache_map_.add(neighb_side);
            }

        element_cache_map_.prepare_elements_to_update();
        this->insert_eval_points_from_integral_data();
        element_cache_map_.create_elements_points_map();
        multidim_assembly_[1_d]->data_->cache_update(element_cache_map_);
        element_cache_map_.finish_elements_update();
    }

    /// Add data of volume integral to appropriate data structure.
    void add_volume_integral(const DHCellAccessor &cell) {
        bulk_integral_data_[ integrals_size_[0] ].cell = cell;
        bulk_integral_data_[ integrals_size_[0] ].subset_index = integrals_.bulk_[cell.dim()-1]->get_subset_idx();
        integrals_size_[0]++;
    }

    /// Add data of edge integral to appropriate data structure.
    void add_edge_integral(RangeConvert<DHEdgeSide, DHCellSide> edge_side_range) {
    	edge_integral_data_[ integrals_size_[1] ].edge_side_range = edge_side_range;
    	edge_integral_data_[ integrals_size_[1] ].subset_index = integrals_.edge_[edge_side_range.begin()->dim()-1]->get_subset_idx();
        integrals_size_[1]++;
    }

    /// Add data of coupling integral to appropriate data structure.
    void add_coupling_integral(const DHCellAccessor &cell, const DHCellSide &ngh_side) {
    	coupling_integral_data_[ integrals_size_[2] ].cell = cell;
    	coupling_integral_data_[ integrals_size_[2] ].side = ngh_side;
    	coupling_integral_data_[ integrals_size_[2] ].bulk_subset_index = integrals_.coupling_[cell.dim()-1]->get_subset_low_idx();
    	coupling_integral_data_[ integrals_size_[2] ].side_subset_index = integrals_.coupling_[cell.dim()-1]->get_subset_high_idx();
        integrals_size_[2]++;
    }

    /// Add data of boundary integral to appropriate data structure.
    void add_boundary_integral(const DHCellSide &bdr_side) {
    	boundary_integral_data_[ integrals_size_[3] ].side = bdr_side;
    	boundary_integral_data_[ integrals_size_[3] ].bdr_subset_index = integrals_.boundary_[bdr_side.dim()-1]->get_subset_low_idx();
    	boundary_integral_data_[ integrals_size_[3] ].side_subset_index = integrals_.boundary_[bdr_side.dim()-1]->get_subset_high_idx();
        integrals_size_[3]++;
    }

    /// Add data of integral of element.center eval point to appropriate data structure.
    void add_center_integral(const DHCellAccessor &cell) {
    	center_integral_data_[ integrals_size_[4] ].cell = cell;
    	center_integral_data_[ integrals_size_[4] ].subset_index = integrals_.center_[cell.dim()]->get_subset_idx();
        integrals_size_[4]++;
    }

    /// Add data of integral of boundary element.center eval point to appropriate data structure.
    void add_bdr_elem_integral(const ElementAccessor<3> elm) {
    	bdr_elem_integral_data_[ integrals_size_[5] ].elm = elm;
    	bdr_elem_integral_data_[ integrals_size_[5] ].subset_index = integrals_.center_[elm.dim()]->get_subset_idx();
        integrals_size_[5]++;
    }


    /// Assembly object
    MixedPtr<DimAssembly, 1> multidim_assembly_;

    /// Holds mask of active integrals.
    int active_integrals_;

    AssemblyIntegrals integrals_;                                 ///< Holds integral objects.
    std::shared_ptr<EvalPoints> eval_points_;                     ///< EvalPoints object shared by all integrals
    ElementCacheMap element_cache_map_;                           ///< ElementCacheMap according to EvalPoints

    // Following variables hold data of all integrals depending of actual computed element.
    std::array<BulkIntegralData, 1>       bulk_integral_data_;      ///< Holds data for computing bulk integrals.
    std::array<EdgeIntegralData, 4>       edge_integral_data_;      ///< Holds data for computing edge integrals.
    std::array<CouplingIntegralData, 6>   coupling_integral_data_;  ///< Holds data for computing couplings integrals.
    std::array<BoundaryIntegralData, 4>   boundary_integral_data_;  ///< Holds data for computing boundary integrals.
    std::array<BulkIntegralData, 11>      center_integral_data_;    ///< Holds data for computing bulk integrals in element.center point.
    std::array<BdrElementIntegralData, 4> bdr_elem_integral_data_;  ///< Holds data for computing boundary integrals in element.center point.
    std::array<unsigned int, 6>           integrals_size_;          ///< Holds used sizes of previous integral data types
};


#endif /* GENERIC_ASSEMBLY_HH_ */