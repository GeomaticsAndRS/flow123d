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
 * @file    bidirectional_map.hh
 * @brief   Implementation of bidirectional map.
 */

#ifndef BIDIRECTIONAL_MAP_HH_
#define BIDIRECTIONAL_MAP_HH_

/**
 * @brief Bidirectional map templated by <T, unsigned int>.
 *
 * Store pairs of value and its position (index) and allow bidirectional search.
 * Items of both (values, positions) must be unique.
 */
template<typename T>
class BidirectionalMap
{
public:
	/// Constructor
	BidirectionalMap();

	/// Return size of map.
	unsigned int size() const;

	/**
	 * Set value of given position.
	 *
	 * Position must be in interval set in \p reinit method <0, init_size-1>.
	 * Element on every position can be set only once.
	 */
	void set_item(T val, unsigned int pos);

	/// Add new item at the end position of map.
	unsigned int add_item(T val);

	/// Return position of item of given value.
	int get_position(T val) const;

	/// Reset data of map, allow reserve size.
	void reinit(unsigned int init_size = 0);

	/// Return value on given position.
	T operator[](unsigned int pos) const;

private:
    std::vector<T> vals_vec_;             ///< Space to save values.
    std::map<T, unsigned int> vals_map_;  ///< Maps values to indexes into vals_vec_.
};

// --------------------------------------------------- BidirectionalMap INLINE implementation -----------
template<typename T>
inline BidirectionalMap<T>::BidirectionalMap()
{}

template<typename T>
inline unsigned int BidirectionalMap<T>::size() const {
	return vals_map_.size();
}

template<typename T>
inline void BidirectionalMap<T>::set_item(T val, unsigned int pos) {
	ASSERT_LT( pos, vals_vec_.size() )(pos)(vals_vec_.size()).error("Value id is out of vector size.");
	ASSERT( vals_vec_[pos] != -1 )(pos).error("Repeated setting of item.");
	vals_map_[val] = pos;
	vals_vec_[pos] = val;
}

template<typename T>
inline unsigned int BidirectionalMap<T>::add_item(T val) {
	ASSERT( vals_map_.find(val) == vals_map_.end() )(val).error("Can not add item since it already exists.");
	vals_map_[val] = vals_vec_.size();
	vals_vec_.push_back(val);
	return vals_map_[val];
}

template<typename T>
inline int BidirectionalMap<T>::get_position(T val) const {
	typename std::map<T, unsigned int>::const_iterator iter = vals_map_.find(val);
	if (iter == vals_map_.end()) return -1;
	else return iter->second;
}

template<typename T>
inline void BidirectionalMap<T>::reinit(unsigned int init_size) {
	vals_map_.clear();
	vals_vec_.clear();
	vals_vec_.resize(init_size, -1);
}

template<typename T>
inline T BidirectionalMap<T>::operator[](unsigned int pos) const {
	ASSERT( pos < vals_vec_.size() );
	return vals_vec_[pos];
}


#endif // BIDIRECTIONAL_MAP_HH_
