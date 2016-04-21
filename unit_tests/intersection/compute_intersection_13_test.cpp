/*
 * intersection_area_test.cpp
 *
 *  Created on: Nov 14, 2014
 *      Author: VF, PE
 */
#define TEST_USE_PETSC
#include <flow_gtest_mpi.hh>

#include "system/system.hh"
#include "system/sys_profiler.hh"
#include "system/file_path.hh"
#include "mesh/mesh.h"
#include "mesh/msh_gmshreader.h"

#include "mesh/ngh/include/point.h"
#include "mesh/ngh/include/intersection.h"

#include "intersection/inspectelements.h"
#include "intersection/intersectionpoint.h"
#include "intersection/intersection_local.h"

#include <dirent.h>

using namespace std;
using namespace computeintersection;

static const std::string profiler_file = "compute_intersection_13d_profiler.log";
static const unsigned int profiler_loop = 1;

// ******************************************************************************************* TEST 1d-2d ****
/*
/// Create results for the meshes in directory 'site_13d'.
void fill_12d_solution(std::vector<std::vector<computeintersection::IntersectionPoint<1,2>>> &ips)
{
    ips.clear();
    ips.resize(9);
    // ips[0] is empty
    ips[1].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({1,0}),arma::vec::fixed<3>({1,0,0})));
    ips[2].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0,1}),arma::vec::fixed<3>({1,0,0})));
    ips[3].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0.5,0.5}),arma::vec::fixed<3>({1,0,0})));
    ips[4].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0.5,0.5}),arma::vec::fixed<3>({1,0,0})));
    ips[5].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0.5,0.5}),arma::vec::fixed<3>({0.5,0.5,0})));
    ips[6].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0.5,0.5}),arma::vec::fixed<3>({0.5,0.25,0.25})));
    ips[7].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({2,1})/3,arma::vec::fixed<3>({0.8,0.2,0})));
    ips[7].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({1,2})/3,arma::vec::fixed<3>({0.6,0,0.4})));
    ips[8].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({1,0}),arma::vec::fixed<3>({1,0,0})));
    ips[8].push_back(computeintersection::IntersectionPoint<1,2>(arma::vec::fixed<2>({0,1}),arma::vec::fixed<3>({0,1,0})));
}


///Permutes tetrahedron coordinates of IP<1,3> according to given permutation.
std::vector<computeintersection::IntersectionPoint<1,2>> permute_coords(std::vector<computeintersection::IntersectionPoint<1,2>> ips, 
                                                                        unsigned int permute[3])
{
    std::vector<computeintersection::IntersectionPoint<1,2>> new_points(ips.size());
    for(unsigned int i = 0; i < ips.size(); i++)
    {
        arma::vec::fixed<3> new_coords;
        for(unsigned int j = 0; j < 3; j++)
            new_coords[j] = ips[i].local_bcoords_B()[permute[j]];
        
        new_points[i].set_coordinates(ips[i].local_bcoords_A(), new_coords);
    }
    return new_points;
}

void compute_intersection_12d(Mesh *mesh, const std::vector<computeintersection::IntersectionPoint<1,2>> &ips)
{
    DBGMSG("Computing 1d-2d intersections.\n");
    InspectElements ie(mesh);
    ie.compute_intersections<1,2>();
    
    //test solution
    std::vector<computeintersection::IntersectionPoint<1,2>> ipc = ie.list_intersection_points(1);

    EXPECT_EQ(ipc.size(), ips.size());
    
    for(unsigned int i=0; i < ipc.size(); i++)
    {
        DBGMSG("---------- check IP[%d] ----------\n",i);
        EXPECT_DOUBLE_EQ(ipc[i].local_bcoords_A()[0], ips[i].local_bcoords_A()[0]);
        EXPECT_DOUBLE_EQ(ipc[i].local_bcoords_A()[1], ips[i].local_bcoords_A()[1]);
        EXPECT_DOUBLE_EQ(ipc[i].local_bcoords_B()[0], ips[i].local_bcoords_B()[0]);
        EXPECT_DOUBLE_EQ(ipc[i].local_bcoords_B()[1], ips[i].local_bcoords_B()[1]);
        EXPECT_DOUBLE_EQ(ipc[i].local_bcoords_B()[2], ips[i].local_bcoords_B()[2]);
    }
}


TEST(intersections_12d, all) {
    Profiler::initialize();
    
    // directory with testing meshes
    string dir_name = string(UNIT_TESTS_SRC_DIR) + "/mesh/meshes_12d/";
    std::vector<string> filenames;
    
    // read mesh file names
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_name.c_str())) != NULL) {
        // print all the files and directories within directory 
        xprintf(Msg,"Testing mesh files: \n");
        while ((ent = readdir (dir)) != NULL) {
            string fname = ent->d_name;
            // test extension ".msh"
            if(fname.size() >= 4)
            {
                string ext = fname.substr(fname.size()-4);
//                 xprintf(Msg,"%s\n",ext.c_str());
                if(ext == ".msh"){
                    filenames.push_back(ent->d_name);
                    xprintf(Msg,"%s\n",ent->d_name);
                }
            }
        }
        closedir (dir);
    } else {
        ASSERT(0,"Could not open directory with testing meshes.");
    }
    
    std::sort(filenames.begin(), filenames.end(), less<string>());
    
    std::vector<std::vector<computeintersection::IntersectionPoint<1,2>>> solution;
    fill_12d_solution(solution);
    
    // for each mesh, compute intersection area and compare with old NGH
    for(unsigned int s=0; s< filenames.size(); s++)
    {
        const unsigned int np = 6;
        unsigned int permutations[np][3] = {{0,1,2},
                                            {1,0,2},
                                            {1,2,0},
                                            {0,2,1},
                                            {2,0,1},
                                            {2,1,0}};
        for(unsigned int p=0; p<np; p++)
        {
            xprintf(Msg,"Computing intersection on mesh: %s\n",filenames[s].c_str());
            FilePath::set_io_dirs(".","","",".");
            FilePath mesh_file(dir_name + filenames[s], FilePath::input_file);
            
            Mesh mesh;
            // read mesh with gmshreader
            GmshMeshReader reader(mesh_file);
            reader.read_mesh(&mesh);
        
            // permute nodes:
            FOR_ELEMENTS(&mesh,ele)
            {
                if(ele->dim() == 2)
                {
                    Node* tmp[3];
                    for(unsigned int i=0; i<ele->n_nodes(); i++)
                    {
                        tmp[i] = ele->node[permutations[p][i]];
                    }
                    for(unsigned int i=0; i<ele->n_nodes(); i++)
                    {
                        ele->node[i] = tmp[i];
//                         ele->node[i]->point().print(cout);
                    }
//                     cout << p << ": jac = "  << ele->tetrahedron_jacobian() << endl;
                }
            }
            
            mesh.setup_topology();
            
            xprintf(Msg, "==============\n");
            for(unsigned int loop = 0; loop < profiler_loop; loop++)
                compute_intersection_12d(&mesh, permute_coords(solution[s], permutations[p]));
            xprintf(Msg, "==============\n");
        }
    }
    std::fstream fs;
    fs.open(profiler_file.c_str(), std::fstream::out | std::fstream::trunc);
    Profiler::instance()->output(PETSC_COMM_WORLD, fs);
    Profiler::uninitialize();
}

//*/


//*

// ******************************************************************************************* TEST 1d-3d ****

/// Create results for the meshes in directory 'simple_meshes_13d'.
void fill_13d_solution(std::vector<computeintersection::IntersectionLocal<1,3>> &ils)
{
    DBGMSG("fill solution\n");
    ils.clear();
    ils.resize(12);
    // ils[0] is empty
    ils[1].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({0,0,0})));
    ils[1].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({1./3}),arma::vec3({1,1,1})/3));
    // only one IP
    ils[2].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({0,0,0})));
    
    ils[3].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({0,0,0})));
    ils[3].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({2./3}),arma::vec3({0,1,0})));
    
    ils[4].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.25}),arma::vec3({0,0.5,0})));
    ils[4].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.5}),arma::vec3({0.5,0.5,0})));
    
    ils[5].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({1./3}),arma::vec3({0,0.5,0})));
    ils[5].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({2./3}),arma::vec3({0.5,0,0})));
    // only one IP
    ils[6].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({0.25,0,0.25})));
    
    ils[7].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({1,1,1})/4));
    ils[7].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.5}),arma::vec3({3,0,3})*0.1));
    
    ils[8].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0}),arma::vec3({1,1,1})*0.2));
    ils[8].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({1}),arma::vec3({1,1,1})*0.3));
    
    // ils[9] is empty
    // only one IP
    ils[10].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.5}),arma::vec3({1,0,0})/2));
    
    ils[11].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.25}),arma::vec3({2,1,0})/4));
    ils[11].points().push_back(computeintersection::IntersectionPoint<1,3>(arma::vec::fixed<1>({0.5}),arma::vec3({2,0,1})/4));

    DBGMSG("fill solution\n");
}


//Permutes tetrahedron coordinates of IP<1,3> according to given permutation.
computeintersection::IntersectionLocal<1,3> permute_coords(computeintersection::IntersectionLocal<1,3> il, unsigned int permute[4])
{
    computeintersection::IntersectionLocal<1,3> new_il = il;
    std::vector<computeintersection::IntersectionPoint<1,3>> & points = il.points();
    for(unsigned int i = 0; i < points.size(); i++)
    {
        arma::vec4 new_coords;
        arma::vec4 old_coords = {1,1,1,1};
        for(unsigned int j = 0; j < 3; j++){
            old_coords[j+1] = points[i].bulk_coords()[j];
            old_coords[0] = old_coords[0] - points[i].bulk_coords()[j];
        }
        if(old_coords[0] < 1e-15) old_coords[0] = 0;
        
        for(unsigned int j = 0; j < 4; j++)
            new_coords[j] = old_coords[permute[j]];
        
        new_il.points()[i] = computeintersection::IntersectionPoint<1,3>(points[i].comp_coords(), new_coords.subvec(1,3));
    }
    return new_il;
}

void compute_intersection_13d(Mesh *mesh, const computeintersection::IntersectionLocal<1,3> &il)
{
    double length1, length2 = 0;

    // compute intersection
    DBGMSG("Computing intersection length by NEW algorithm\n");
    InspectElements ie(mesh);
    ie.compute_intersections(computeintersection::IntersectionType::d13);
    
    //test solution
    std::vector<computeintersection::IntersectionLocal<1,3>> pp = ie.intersection_storage13_;
    computeintersection::IntersectionLocal<1,3> ilc;
    // component = element index == 1
    if(pp.size() > 0)
    {
        ilc = pp[0];
        EXPECT_EQ(ilc.size(), il.size());
    }
    
    for(unsigned int i=0; i < ilc.size(); i++)
    {
        DBGMSG("---------- check IP[%d] ----------\n",i);
        EXPECT_DOUBLE_EQ(ilc[i].comp_coords()[0], il[i].comp_coords()[0]);
        EXPECT_DOUBLE_EQ(ilc[i].bulk_coords()[0], il[i].bulk_coords()[0]);
        EXPECT_DOUBLE_EQ(ilc[i].bulk_coords()[1], il[i].bulk_coords()[1]);
        EXPECT_DOUBLE_EQ(ilc[i].bulk_coords()[2], il[i].bulk_coords()[2]);
    }
    
    length1 = ie.measure_13();
        ie.print_mesh_to_file_13("output_intersection_13");
    
    //TODO: delete comparison with NGH
    // compute intersection by NGH
    DBGMSG("Computing intersection length by NGH algorithm\n");
    START_TIMER("OLD intersections 1D-3D");
    TAbscissa tabs;
    TTetrahedron tte;
    TIntersectionType it = line;

    FOR_ELEMENTS(mesh, elm) {
        if (elm->dim() == 1) {
        tabs.SetPoints(TPoint(elm->node[0]->point()(0), elm->node[0]->point()(1), elm->node[0]->point()(2)),
                       TPoint(elm->node[1]->point()(0), elm->node[1]->point()(1), elm->node[1]->point()(2)));
        }
        else if(elm->dim() == 3){
        tte.SetPoints(TPoint(elm->node[0]->point()(0), elm->node[0]->point()(1), elm->node[0]->point()(2)),
                     TPoint(elm->node[1]->point()(0), elm->node[1]->point()(1), elm->node[1]->point()(2)),
                     TPoint(elm->node[2]->point()(0), elm->node[2]->point()(1), elm->node[2]->point()(2)),
                     TPoint(elm->node[3]->point()(0), elm->node[3]->point()(1), elm->node[3]->point()(2)));
        }
    }
    GetIntersection(tabs, tte, it, length2); // get only relative length of the intersection to the abscissa
    length2 *= tabs.Length(); 
    END_TIMER("OLD intersections 1D-3D");
    
    DBGMSG("Length of intersection line: (intersections) %.16e,\t(NGH) %.16e\n", length1, length2);
//     EXPECT_NEAR(length1, length2, 1e-12);
    EXPECT_DOUBLE_EQ(length1,length2);
}


TEST(intersections_13d, all) {
    Profiler::initialize();
    
    // directory with testing meshes
    string dir_name = string(UNIT_TESTS_SRC_DIR) + "/intersection/simple_meshes_13d/";
    std::vector<string> filenames;
    
    // read mesh file names
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (dir_name.c_str())) != NULL) {
        // print all the files and directories within directory 
        xprintf(Msg,"Testing mesh files: \n");
        while ((ent = readdir (dir)) != NULL) {
            string fname = ent->d_name;
            // test extension ".msh"
            if(fname.size() >= 4)
            {
                string ext = fname.substr(fname.size()-4);
//                 xprintf(Msg,"%s\n",ext.c_str());
                if(ext == ".msh"){
                    filenames.push_back(ent->d_name);
                    xprintf(Msg,"%s\n",ent->d_name);
                }
            }
        }
        closedir (dir);
    } else {
        ASSERT(0,"Could not open directory with testing meshes.");
    }
    
    std::sort(filenames.begin(), filenames.end(), less<string>());
    
    std::vector<computeintersection::IntersectionLocal<1,3>> solution;
    fill_13d_solution(solution);
    
    // for each mesh, compute intersection area and compare with old NGH
    for(unsigned int s=0; s< filenames.size(); s++)
    {
        const unsigned int np = 12;
        unsigned int permutations[np][4] = {{0,1,2,3},
//                                                 {0,1,3,2},  // the tab means permutation with negative jacobian
                                            {0,3,1,2},
//                                                 {0,3,2,1},
                                            {0,2,3,1},
//                                                 {0,2,1,3},
//                                                 {1,0,2,3},
                                            {1,0,3,2},
//                                                 {1,3,0,2},
                                            {1,3,2,0},
//                                                 {1,2,3,0},
                                            {1,2,0,3},
//                                                 {2,1,0,3},
                                            {2,1,3,0},
//                                                 {2,3,1,0},
                                            {2,3,0,1},
//                                                 {2,0,3,1},
                                            {2,0,1,3},
//                                                 {3,1,2,0},
                                            {3,1,0,2},
//                                                 {3,0,1,2},
                                            {3,0,2,1},
//                                                 {3,2,0,1},
                                            {3,2,1,0}};
        for(unsigned int p=0; p<np; p++)
        {
            xprintf(Msg,"Computing intersection on mesh: %s\n",filenames[s].c_str());
            FilePath::set_io_dirs(".","","",".");
            FilePath mesh_file(dir_name + filenames[s], FilePath::input_file);
            
            Mesh mesh;
            // read mesh with gmshreader
            GmshMeshReader reader(mesh_file);
            reader.read_mesh(&mesh);
        
            // permute nodes:
            FOR_ELEMENTS(&mesh,ele)
            {
                if(ele->dim() == 3)
                {
                    Node* tmp[4];
                    for(unsigned int i=0; i<ele->n_nodes(); i++)
                    {
                        tmp[i] = ele->node[permutations[p][i]];
                    }
                    for(unsigned int i=0; i<ele->n_nodes(); i++)
                    {
                        ele->node[i] = tmp[i];
//                         ele->node[i]->point().print(cout);
                    }
//                     cout << p << ": jac = "  << ele->tetrahedron_jacobian() << endl;
                }
            }
            
            mesh.setup_topology();
            
            xprintf(Msg, "==============\n");
            for(unsigned int loop = 0; loop < profiler_loop; loop++)
                compute_intersection_13d(&mesh, permute_coords(solution[s], permutations[p]));
            xprintf(Msg, "==============\n");
        }
    }
    std::fstream fs;
    fs.open(profiler_file.c_str(), std::fstream::out | std::fstream::app);
    Profiler::instance()->output(PETSC_COMM_WORLD, fs);
    Profiler::uninitialize();
}

//*/