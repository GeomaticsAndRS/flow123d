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
 * @file    field_value_cache.hh
 * @brief
 * @author  David Flanderka
 */

#ifndef FIELD_VALUE_CACHE_HH_
#define FIELD_VALUE_CACHE_HH_

class EvalPoints;


template<class Value>
class FieldValueCache {
public:
    /// Number of cached elements which values are stored in cache.
    static const unsigned int n_cached_elements;

    /// Constructor
    FieldValueCache(EvalPoints eval_points);

    /// Destructor
   ~FieldValueCache();
private:
    /// Data cache
    double *data_;
};



#endif /* FIELD_VALUE_CACHE_HH_ */
