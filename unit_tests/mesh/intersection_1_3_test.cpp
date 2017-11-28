/*
 * muj_test.cpp
 *
 *  Created on: 6.2.2013
 *      Author: viktor
 */

#define FEAL_OVERRIDE_ASSERTS

#include <flow_gtest.hh>
#include <mesh_constructor.hh>
#include "system/system.hh"
#include "system/sys_profiler.hh"
#include "system/file_path.hh"
#include "mesh/ngh/include/point.h"
#include "mesh/ngh/include/intersection.h"
#include "mesh/mesh.h"
#include "io/msh_gmshreader.h"
#include "mesh/bih_tree.hh"
#include "fields/field_interpolated_p0.hh"
#include "system/sys_profiler.hh"

//#include "new_mesh/ngh/include/point.h"

#define FLOW123D_DEBUG

// Test rychlosti algoritmu, pro vyhledávání průsečíků sítě line_cube.msh
TEST(intersections, 1d_3d){
        Profiler::initialize();
	unsigned int elementLimit = 20;
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"", ".");
    std::string mesh_input_string = "{mesh_file=\"mesh/line_cube.msh\"}";

	Mesh * mesh_krychle = mesh_constructor(mesh_input_string);
    auto reader = reader_constructor(mesh_input_string);
    reader->read_physical_names(mesh_krychle);
    reader->read_raw_mesh(mesh_krychle);

	BoundingBox bb;
	std::vector<unsigned int> searchedElements;

	BIHTree bt(mesh_krychle.get(), elementLimit);

	//Profiler::initialize();
	{
	    START_TIMER("Inter");

	    FOR_ELEMENTS(mesh_krychle.get(), elm) {
	         if (elm->dim() == 1) {
	        	TAbscissa ta;
	        	FieldInterpolatedP0<3,FieldValue<3>::Scalar>::create_abscissa(elm, ta);
	        	BoundingBox elementBoundingBox = ta.get_bounding_box();
	        	bt.find_bounding_box(elementBoundingBox, searchedElements);
	        	TTetrahedron tt;
	        	TIntersectionType iType;
	        	double measure;

	        	for (std::vector<unsigned int>::iterator it = searchedElements.begin(); it!=searchedElements.end(); it++)
	        		{
	        			int idx = *it;
	        			ElementFullIter ele = mesh_krychle->element( idx );
	        			if (ele->dim() == 3) {
	        				FieldInterpolatedP0<3,FieldValue<3>::Scalar>::create_tetrahedron(ele, tt);
	        				GetIntersection(ta, tt, iType, measure);
	        				/*if (iType == line) {
	        					MessageOut().fmt("{} {} \n",elm.id(),ele.id());
	        				              }*/

	        			}
	        		}

	         }
	     }
	}
        
	Profiler::instance()->output(MPI_COMM_WORLD, cout);
        
	Profiler::uninitialize();

	MessageOut() << "Test is complete\n";

	delete mesh_krychle;
}



