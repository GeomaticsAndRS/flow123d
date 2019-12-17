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
 * @file    eval_subset.hh
 * @brief
 * @author  David Flanderka
 */

#ifndef EVAL_SUBSET_HH_
#define EVAL_SUBSET_HH_

#include <memory>
#include <armadillo>
#include "fields/eval_points.hh"
#include "mesh/range_wrapper.hh"
#include "fem/dh_cell_accessor.hh"


class Side;
class BulkPoint;
class SidePoint;


/**
 * @brief Class holds set of bulk or side local points specified by dimension.
 *
 * It provides methods allows iterate through local points. Iterating on side-points
 * set is possible over individual sides and different permutations.
 */
class EvalSubset : public std::enable_shared_from_this<EvalSubset> {
public:
	TYPEDEF_ERR_INFO(EI_ElementIdx, unsigned int);
	DECLARE_EXCEPTION(ExcElementNotInCache,
	        << "Element of Idx: " << EI_ElementIdx::val << " is not stored in 'Field value data cache'.\n"
			   << "Value can't be computed.\n");

    /// Default constructor
	EvalSubset() : eval_points_(nullptr), perm_indices_(nullptr), n_permutations_(0) {}

    /// Constructor of bulk (n_permutations==0) or side subset
	EvalSubset(std::shared_ptr<EvalPoints> eval_points, unsigned int n_permutations = 0, unsigned int points_per_side = 0);

    /// Destructor
	~EvalSubset();

    /// Getter of eval_points
    inline std::shared_ptr<EvalPoints> eval_points() const {
        return eval_points_;
    }

    /// Getter of n_sides
    inline const unsigned int n_sides() const {
        return n_sides_;
    }

    /// Return index of data block according to subset in EvalPoints object
    inline int get_subset_idx() const {
        return subset_index_;
    }

    /// Returns range of bulk local points for appropriate cell accessor
    Range< BulkPoint > points(const DHCellAccessor &cell) const;

    /// Returns range of side local points for appropriate cell side accessor
    Range< SidePoint > points(const DHCellSide &cell_side) const;

    /// Returns structure of permutation indices.
    inline int perm_idx_ptr(uint i_side, uint i_perm, uint i_point) const {
    	return perm_indices_[i_side][i_perm][i_point];
    }

private:
    /// Pointer to EvalPoints
    std::shared_ptr<EvalPoints> eval_points_;
    /// Index of data block according to subset in EvalPoints object.
    unsigned int subset_index_;
    /// Indices to EvalPoints for different sides and permutations reflecting order of points.
    unsigned int*** perm_indices_;
    /// Number of sides (value 0 indicates bulk set)
    unsigned int n_sides_;
    /// Number of permutations (value 0 indicates bulk set)
    unsigned int n_permutations_;

    friend class EvalPoints;
};


/**
 * @brief Point accessor allow iterate over bulk quadrature points defined in local element coordinates.
 */
class BulkPoint {
public:
    /// Default constructor
	BulkPoint()
    : local_point_idx_(0) {}

    /// Constructor
	BulkPoint(DHCellAccessor dh_cell, std::shared_ptr<const EvalSubset> bulk_subset, unsigned int loc_point_idx)
    : dh_cell_(dh_cell), subset_(bulk_subset), local_point_idx_(loc_point_idx) {}

    /// Getter of EvalSubset
    inline std::shared_ptr<const EvalSubset> eval_subset() const {
        return subset_;
    }

    /// Getter of EvalPoints
    inline std::shared_ptr<EvalPoints> eval_points() const {
        return subset_->eval_points();
    }

    /// Local coordinates within element
    inline arma::vec loc_coords() const {
        return this->eval_points()->local_point( local_point_idx_ );
    }

    // Global coordinates within element
    //arma::vec3 coords() const;

    /// Return index of element in data cache.
    inline unsigned int element_cache_index() const {
        return dh_cell_.element_cache_index();
    }

    /// Return DH cell accessor.
    inline DHCellAccessor dh_cell() const {
        return dh_cell_;
    }

    /// Return index in EvalPoints object
    inline unsigned int eval_point_idx() const {
        return local_point_idx_;
    }

    /// Iterates to next point.
    inline void inc() {
    	local_point_idx_++;
    }

    /// Comparison of accessors.
    bool operator==(const BulkPoint& other) {
    	return (dh_cell_ == other.dh_cell_) && (local_point_idx_ == other.local_point_idx_);
    }

private:
    /// DOF handler accessor of element.
    DHCellAccessor dh_cell_;
    /// Pointer to bulk point set.
    std::shared_ptr<const EvalSubset> subset_;
    /// Index of the local point in bulk point set.
    unsigned int local_point_idx_;
};


/**
 * @brief Point accessor allow iterate over quadrature points of given side defined in local element coordinates.
 */
class SidePoint {
public:
    /// Default constructor
	SidePoint()
    : local_point_idx_(0) {}

    /// Constructor
	SidePoint(DHCellSide cell_side, std::shared_ptr<const EvalSubset> subset, unsigned int local_point_idx)
    : cell_side_(cell_side), subset_(subset), local_point_idx_(local_point_idx),
	  permutation_idx_( cell_side.element()->permutation_idx( cell_side_.side_idx() ) ) {}

    /// Getter of EvalSubset
    inline std::shared_ptr<const EvalSubset> eval_subset() const {
        return subset_;
    }

    /// Getter of evaluation points
    inline std::shared_ptr<EvalPoints> eval_points() const {
        return subset_->eval_points();
    }

    // Local coordinates within element
    inline arma::vec loc_coords() const {
        return this->eval_points()->local_point( this->eval_point_idx() );
    }

    // Global coordinates within element
    //arma::vec3 coords() const;

    /// Return index of element in data cache.
    inline unsigned int element_cache_index() const {
        return cell_side_.cell().element_cache_index();
    }

    /// Return DH cell accessor.
    inline DHCellSide dh_cell_side() const {
        return cell_side_;
    }

    // Index of permutation
    inline unsigned int permutation_idx() const {
        return permutation_idx_;
    }

    /// Return index in EvalPoints object
    inline unsigned int eval_point_idx() const {
        return subset_->perm_idx_ptr(cell_side_.side_idx(), permutation_idx_, local_point_idx_);
    }

    /// Return corresponds SidePoints of neighbour side of same dimension (computing of side integrals).
    SidePoint permute(DHCellSide edg_side) const;

    /// Iterates to next point.
    inline void inc() {
    	local_point_idx_++;
    }

    /// Comparison of accessors.
    bool operator==(const SidePoint& other) {
    	return (cell_side_ == other.cell_side_) && (local_point_idx_ == other.local_point_idx_);
    }

private:
    /// DOF handler accessor of element side.
    DHCellSide cell_side_;
    /// Pointer to side point set
    std::shared_ptr<const EvalSubset> subset_;
    /// Index of the local point in the composed quadrature.
    unsigned int local_point_idx_;
    /// Permutation index corresponding with DHCellSide
    unsigned int permutation_idx_;
};


#endif /* EVAL_SUBSET_HH_ */