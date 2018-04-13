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
 * @file    mesh.h
 * @brief   
 */

#ifndef MAKE_MESH_H
#define MAKE_MESH_H

#include <mpi.h>                             // for MPI_Comm, MPI_COMM_WORLD
#include <boost/exception/info.hpp>          // for error_info::~error_info<...
#include <memory>                            // for shared_ptr
#include <string>                            // for string
#include <vector>                            // for vector, vector<>::iterator
#include "input/accessors.hh"                // for Record, Array (ptr only)
#include "input/accessors_impl.hh"           // for Record::val
#include "input/storage.hh"                  // for ExcStorageTypeMismatch
#include "input/type_record.hh"              // for Record (ptr only), Recor...
#include "mesh/boundaries.h"                 // for Boundary
#include "mesh/edges.h"                      // for Edge
#include "mesh/mesh_types.hh"                // for ElementVector, ElementFu...
#include "mesh/neighbours.h"                 // for Neighbour
#include "mesh/region.hh"                    // for RegionDB, RegionDB::MapE...
#include "mesh/sides.h"                      // for SideIter
#include "mesh/bounding_box.hh"              // for BoundingBox
#include "system/exceptions.hh"              // for operator<<, ExcStream, EI
#include "system/file_path.hh"               // for FilePath
#include "system/sys_vector.hh"              // for FullIterator, VectorId<>...

class BIHTree;
class Distribution;
class Partitioning;
template <int spacedim> class ElementAccessor;


#define ELM  0
#define BC  1
#define NODE  2

/**
 *  This parameter limits volume of elements from below.
 */
#define MESH_CRITICAL_VOLUME 1.0E-12

/**
 * Provides for statement to iterate over the Nodes of the Mesh.
 * The parameter is FullIter local variable of the cycle, so it need not be declared before.
 * Macro assume that variable Mesh *mesh; is declared and points to a valid Mesh structure.
 */
#define FOR_NODES(_mesh_, i) \
    for( NodeFullIter i( (_mesh_)->node_vector.begin() ); \
        i != (_mesh_)->node_vector.end(); \
        ++i)

/**
 * Macro for conversion form Iter to FullIter for nodes.
 */
#define NODE_FULL_ITER(_mesh_,i) \
    (_mesh_)->node_vector.full_iter(i)

/**
 * Macro to get "NULL" ElementFullIter.
 */
#define NODE_FULL_ITER_NULL(_mesh_) \
    NodeFullIter((_mesh_)->node_vector)

/**
 * Macro for conversion form Iter to FullIter for elements.
 */
#define ELEM_FULL_ITER(_mesh_,i) \
    (_mesh_)->element.full_iter(i)


#define FOR_NODE_ELEMENTS(i,j)   for((j)=0;(j)<(i)->n_elements();(j)++)
#define FOR_NODE_SIDES(i,j)      for((j)=0;(j)<(i)->n_sides;(j)++)


/// Define integers that are indices into large arrays (elements, nodes, dofs etc.)
typedef int IdxInt;


class BoundarySegment {
public:
    static Input::Type::Record input_type;
};



//=============================================================================
// STRUCTURE OF THE MESH
//=============================================================================

class Mesh {
public:
    TYPEDEF_ERR_INFO( EI_ElemLast, int);
    TYPEDEF_ERR_INFO( EI_ElemNew, int);
    TYPEDEF_ERR_INFO( EI_RegLast, std::string);
    TYPEDEF_ERR_INFO( EI_RegNew, std::string);
    DECLARE_EXCEPTION(ExcDuplicateBoundary,
            << "Duplicate boundary elements! \n"
            << "Element id: " << EI_ElemLast::val << " on region name: " << EI_RegLast::val << "\n"
            << "Element id: " << EI_ElemNew::val << " on region name: " << EI_RegNew::val << "\n");


    /**
     * \brief Types of search algorithm for finding intersection candidates.
     */
    typedef enum IntersectionSearch {
        BIHsearch  = 1,
        BIHonly = 2,
        BBsearch = 3
    } IntersectionSearch;
    
    /**
     * \brief The definition of input record for selection of variant of file format
     */
    static const Input::Type::Selection & get_input_intersection_variant();
    
    static const unsigned int undef_idx=-1;
    static const Input::Type::Record & get_input_type();



    /** Labels for coordinate indexes in arma::vec3 representing vectors and points.*/
    enum {x_coord=0, y_coord=1, z_coord=2};

    /**
     * Empty constructor.
     *
     * Use only for unit tests!!!
     */
    Mesh();
    /**
     * Constructor from an input record.
     * Do not process input record. That is done in init_from_input.
     */
    Mesh(Input::Record in_record, MPI_Comm com = MPI_COMM_WORLD);
    /**
     * Common part of both previous constructors and way how to reinitialize a mesh from the  given input record.
     */
    void reinit(Input::Record in_record);

    /// Destructor.
    ~Mesh();

    inline unsigned int n_nodes() const {
        return node_vector.size();
    }

    inline unsigned int n_elements() const {
        return element.size();
    }

    inline unsigned int n_boundaries() const {
        return boundary_.size();
    }

    inline unsigned int n_edges() const {
        return edges.size();
    }

    unsigned int n_corners();

    inline const RegionDB &region_db() const {
        return region_db_;
    }

    /// Reserve size of node vector
    inline void reserve_node_size(unsigned int n_nodes) {
    	node_vector.reserve(n_nodes);
    }

    /// Reserve size of element vector
    inline void reserve_element_size(unsigned int n_elements) {
    	element.reserve(n_elements);
    }

    /**
     * Returns pointer to partitioning object. Partitioning is created during setup_topology.
     */
    Partitioning *get_part();

    Distribution *get_el_ds() const
    { return el_ds; }

    IdxInt *get_row_4_el() const
    { return row_4_el; }

    IdxInt *get_el_4_loc() const
    { return el_4_loc; }

    /**
     * Returns MPI communicator of the mesh.
     */
    inline MPI_Comm get_comm() const { return comm_; }


    MixedMeshIntersections &mixed_intersections();

    unsigned int n_sides();

    inline unsigned int n_vb_neighbours() const {
        return vb_neighbours_.size();
    }

    /**
     * Returns maximal number of sides of one edge, which connects elements of dimension @p dim.
     * @param dim Dimension of elements sharing the edge.
     */
    unsigned int max_edge_sides(unsigned int dim) const { return max_edge_sides_[dim-1]; }

    /**
     * Reads mesh from stream.
     *
     * Method is especially used in unit tests.
     */
    void read_gmsh_from_stream(istream &in);
    /**
     * Reads input record, creates regions, read the mesh, setup topology. creates region sets.
     */
    void init_from_input();


    /**
     * Initialize all mesh structures from raw information about nodes and elements (including boundary elements).
     * Namely: create remaining boundary elements and Boundary objects, find edges and compatible neighborings.
     */
    void setup_topology();
    
    /**
     * Returns vector of ID numbers of elements, either bulk or bc elemnts.
     */
    void elements_id_maps( vector<IdxInt> & bulk_elements_id, vector<IdxInt> & boundary_elements_id) const;


    ElementAccessor<3> element_accessor(unsigned int idx, bool boundary=false);

    /**
     * Reads elements and their affiliation to regions and region sets defined by user in input file
     * Format of input record is defined in method RegionSetBase::get_input_type()
     *
     * @param region_list Array input AbstractRecords which define regions, region sets and elements
     */
    void read_regions_from_input(Input::Array region_list);

    /**
     * Returns nodes_elements vector, if doesn't exist creates its.
     */
    vector<vector<unsigned int> > const & node_elements();

    /// Vector of nodes of the mesh.
    NodeVector node_vector;
    /// Vector of elements of the mesh.
    ElementVector element;

    /// Vector of boundary sides where is prescribed boundary condition.
    /// TODO: apply all boundary conditions in the main assembling cycle over elements and remove this Vector.
    vector<Boundary> boundary_;
    /// vector of boundary elements - should replace 'boundary'
    /// TODO: put both bulk and bc elements (on zero level) to the same vector or make better map id->element for field inputs that use element IDs
    /// the avoid usage of ElementVector etc.
    ElementVector bc_elements;

    /// Vector of MH edges, this should not be part of the geometrical mesh
    std::vector<Edge> edges;

    //flow::VectorId<int> bcd_group_id; // gives a index of group for an id

    /**
     * Vector of individual intersections of two elements.
     * This is enough for local mortar.
     */
    std::shared_ptr<MixedMeshIntersections>  intersections;

    /**
     * For every element El we have vector of indices into @var intersections array for every intersection in which El is master element.
     * This is necessary for true mortar.
     */
    vector<vector<unsigned int> >  master_elements;

    /**
     * Vector of compatible neighbourings.
     */
    vector<Neighbour> vb_neighbours_;

    int n_insides; // # of internal sides
    int n_exsides; // # of external sides
    int n_sides_; // total number of sides (should be easy to count when we have separated dimensions

    int n_lines; // Number of line elements
    int n_triangles; // Number of triangle elements
    int n_tetrahedras; // Number of tetrahedra elements

    // Temporary solution for numbering of nodes on sides.
    // The data are defined in RefElement<dim>::side_nodes,
    // Mesh::side_nodes can be removed as soon as Element
    // is templated by dimension.
    //
    // for every side dimension D = 0 .. 2
    // for every element side 0 .. D+1
    // for every side node 0 .. D
    // index into element node array
    vector< vector< vector<unsigned int> > > side_nodes;

    /**
     * Check usage of regions, set regions to elements defined by user, close RegionDB
     */
    void check_and_finish();
    
    /// Compute bounding boxes of elements contained in mesh.
    std::vector<BoundingBox> get_element_boxes();

    /// Getter for BIH. Creates and compute BIH at first call.
    const BIHTree &get_bih_tree();\

    /**
     * Find intersection of element lists given by Mesh::node_elements_ for elements givne by @p nodes_list parameter.
     * The result is placed into vector @p intersection_element_list. If the @p node_list is empty, and empty intersection is
     * returned.
     */
    void intersect_element_lists(vector<unsigned int> const &nodes_list, vector<unsigned int> &intersection_element_list);

    /// Add new node of given id and coordinates to mesh
    void add_node(unsigned int node_id, arma::vec3 coords);

    /// Add new element of given id to mesh
    void add_element(unsigned int elm_id, unsigned int dim, unsigned int region_id, unsigned int partition_id,
    		std::vector<unsigned int> node_ids);

    /// Add new node of given id and coordinates to mesh
    void add_physical_name(unsigned int dim, unsigned int id, std::string name);

    /// Return FilePath object representing "mesh_file" input key
    inline FilePath mesh_file() {
    	return in_record_.val<FilePath>("mesh_file");
    }

    /// Getter for input type selection for intersection search algorithm.
    IntersectionSearch get_intersection_search();

    /// Maximal distance of observe point from Mesh relative to its size
    double global_snap_radius() const;

    /// Number of elements read from input.
    unsigned int n_all_input_elements_;


protected:

    /**
     *  This replaces read_neighbours() in order to avoid using NGH preprocessor.
     *
     *  TODO:
     *  - Avoid maps:
     *
     *    4) replace EdgeVector by std::vector<Edge> (need not to know the size)
     *
     *    5) need not to have temporary array for Edges, only postpone setting pointers in elements and set them
     *       after edges are found; we can temporary save Edge index instead of pointer in Neigbours and elements
     *
     *    6) Try replace Edge * by indexes in Neigbours and elements (anyway we have mesh pointer in elements so it is accessible also from Neigbours)
     *
     */
    void make_neighbours_and_edges();

    /**
     * On edges sharing sides of many elements it may happen that each side has its nodes ordered in a different way.
     * This method finds the permutation for each side so as to obtain the ordering of side 0.
     */
    void make_edge_permutations();
    /**
     * Create element lists for nodes in Mesh::nodes_elements.
     */
    void create_node_element_lists();
    /**
     * Remove elements with dimension not equal to @p dim from @p element_list. Index of the first element of dimension @p dim-1,
     * is returned in @p element_idx. If no such element is found the method returns false, if one such element is found the method returns true,
     * if more elements are found we report an user input error.
     */
    bool find_lower_dim_element(ElementVector&elements, vector<unsigned int> &element_list, unsigned int dim, unsigned int &element_idx);

    /**
     * Returns true if side @p si has same nodes as in the list @p side_nodes.
     */
    bool same_sides(const SideIter &si, vector<unsigned int> &side_nodes);


    void element_to_neigh_vb();

    void count_element_types();
    void count_side_types();

    /**
     * Possibly modify region id of elements sets by user in "regions" part of input file.
     *
     * TODO: This method needs check in issue 'Review mesh setting'.
     * Changes have been done during generalized region key and may be causing problems
     * during the further development.
     */
    void modify_element_ids(const RegionDB::MapElementIDToRegionID &map);

    unsigned int n_bb_neigh, n_vb_neigh;

    /// Maximal number of sides per one edge in the actual mesh (set in make_neighbours_and_edges()).
    unsigned int max_edge_sides_[3];

    /// Output of neighboring data into raw output.
    void output_internal_ngh_data();
    
    /**
     * Database of regions (both bulk and boundary) of the mesh. Regions are logical parts of the
     * domain that allows setting of different data and boundary conditions on them.
     */
    RegionDB region_db_;
    /**
     * Mesh partitioning. Created in setup_topology.
     */
    std::shared_ptr<Partitioning> part_;

    /**
     * BIH Tree for intersection and observe points lookup.
     */
    std::shared_ptr<BIHTree> bih_tree_;


    /**
     * Accessor to the input record for the mesh.
     */
    Input::Record in_record_;

    /**
     * MPI communicator used for partitioning and ...
     */
    MPI_Comm comm_;

    // For each node the vector contains a list of elements that use this node
    vector<vector<unsigned int> > node_elements_;


    friend class RegionSetBase;
    friend class Element;
    friend class BIHTree;



private:

    /// Index set assigning to global element index the local index used in parallel vectors.
    IdxInt *row_4_el;
	/// Index set assigning to local element index its global index.
    IdxInt *el_4_loc;
	/// Parallel distribution of elements.
	Distribution *el_ds;
        
    ofstream raw_ngh_output_file;
};


#include "mesh/side_impl.hh"
#include "mesh/element_impls.hh"
#include "mesh/neighbours_impl.hh"

/**
 * Provides for statement to iterate over the Elements of the Mesh.
 * The parameter is FullIter local variable of the cycle, so it need not be declared before.
 * Macro assume that variable Mesh *mesh; is declared and points to a valid Mesh structure.
 */
#define FOR_ELEMENTS(_mesh_,__i) \
    for( ElementFullIter __i( (_mesh_)->element.begin() ); \
        __i != (_mesh_)->element.end(); \
        ++__i)

/**
 * Macro for conversion form Iter to FullIter for elements.
 */
#define ELEMENT_FULL_ITER(_mesh_,i) \
    (_mesh_)->element.full_iter(i)

/**
 * Macro to get "NULL" ElementFullIter.
 */
#define ELEMENT_FULL_ITER_NULL(_mesh_) \
    ElementFullIter((_mesh_)->element)


#define FOR_BOUNDARIES(_mesh_,i) \
for( std::vector<Boundary>::iterator i= (_mesh_)->boundary_.begin(); \
    i != (_mesh_)->boundary_.end(); \
    ++i)

/**
 * Macro for conversion form Iter to FullIter for boundaries.
 */
#define BOUNDARY_FULL_ITER(_mesh_,i) \
    (_mesh_)->boundary.full_iter(i)

/**
 * Macro to get "NULL" BoundaryFullIter.
 */
#define BOUNDARY_NULL(_mesh_) \
    BoundaryFullIter((_mesh_)->boundary)


/**
 * Provides for statement to iterate over the Edges of the Mesh. see FOR_ELEMENTS
 */
#define FOR_EDGES(_mesh_,__i) \
    for( vector<Edge>::iterator __i = (_mesh_)->edges.begin(); \
        __i !=(_mesh_)->edges.end(); \
        ++__i)

#define FOR_SIDES(_mesh_, it) \
    FOR_ELEMENTS((_mesh_), ele)  \
        for(SideIter it = ele->side(0); it->el_idx() < ele->n_sides(); ++it)

#define FOR_SIDE_NODES(i,j) for((j)=0;(j)<(i)->n_nodes;(j)++)


#define FOR_NEIGHBOURS(_mesh_, it) \
    for( std::vector<Neighbour>::iterator it = (_mesh_)->vb_neighbours_.begin(); \
         (it)!= (_mesh_)->vb_neighbours_.end(); ++it)

#define FOR_NEIGH_ELEMENTS(i,j) for((j)=0;(j)<(i)->n_elements;(j)++)
#define FOR_NEIGH_SIDES(i,j)    for((j)=0;(j)<(i)->n_sides;(j)++)


#endif
//-----------------------------------------------------------------------------
// vim: set cindent:
