/*
 * fe_values_test.cpp
 *
 *  Created on: Sep 9, 2012
 *      Author: jb
 */

#define FEAL_OVERRIDE_ASSERTS

#include <flow_gtest.hh>
#include <cmath>
#include "arma_expect.hh"
#include "armadillo"
#include "system/armadillo_tools.hh"
#include "system/sys_profiler.hh"
#include "quadrature/quadrature_lib.hh"
#include "fem/fe_p.hh"
#include "fem/fe_values.hh"
#include "fem/mapping_p1.hh"
#include "mesh/mesh.h"
#include "mesh/element_impls.hh"
#include "mesh/region.hh"

#define INTEGRATE( _func_ ) for( unsigned int i=0; i < quad.size(); i++) sum +=  _func_( quad.point(i) ) * quad.weight(i);

double test_1_1d( const arma::vec::fixed<1> & p) {
    return 3 * p[0] + 1.0;
}

template <int dim>
double func( const arma::vec::fixed<dim> & p) {
    if (dim == 1) {
        return 3 * p[0] * p[0] + p[0] + 1.0;
    } else {
        return 3 * p[0] * p[0] + p[0] + 6 * p[1] * p[1] + p[1] + 1.0;
    }
}


template <int dim>
double integrate(ElementFullIter &ele) {
    FE_P_disc<0,dim,3> fe;
    QGauss<dim> quad( 2 );
    MappingP1<dim,3> map;
    FEValues<dim,3> fe_values(map, quad,   fe, update_JxW_values | update_quadrature_points);

    fe_values.reinit( ele );

    double sum = 0.0;
    for(unsigned int i_point=0; i_point < fe_values.n_points(); i_point++) {
        sum += func<dim>( quad.point(i_point) ) * fe_values.JxW(i_point);
    }
    return sum;
}


TEST(FeValues, test_all) {
  // integrate a polynomial defined on the ref. element over an arbitrary element
    {
        // 1d case interval (1,3)   det(jac) = 2
        NodeVector nodes(2);
        nodes.add_item(0);
        nodes[0].point()[0] = 1.0;
        nodes[0].point()[1] = 0.0;
        nodes[0].point()[2] = 0.0;

        nodes.add_item(1);
        nodes[1].point()[0] = 3.0;
        nodes[1].point()[1] = 0.0;
        nodes[1].point()[2] = 0.0;

        ElementVector el_vec(1);
        el_vec.add_item(0);

        RegionIdx reg;
        Element ele(1, NULL, reg);      //NULL - mesh pointer, empty RegionIdx

        ele.node = new Node * [ele.n_nodes()];
        for(int i =0; i < 2; i++) ele.node[i] = nodes(i);
        el_vec[0] = ele; // dangerous since Element has no deep copy constructor.


        ElementFullIter it( el_vec(0) );
        EXPECT_DOUBLE_EQ( 2.5 * 2, integrate<1>( it ) );

        // projection methods
        MappingP1<1,3> mapping;
        arma::mat::fixed<3, 2> map = mapping.element_map(ele);
        EXPECT_ARMA_EQ( arma::mat("1 2; 0 0; 0 0"), map);
        EXPECT_ARMA_EQ( arma::vec("0.5 0.5"), mapping.project_point( arma::vec("2.0 0.0 0.0"), map ) );

    }

    {
        // 2d case: triangle (0,1) (2,0) (3,4) surface = 3*4 - 1*2/2 - 1*4/4 - 3*3/2 = 9/2, det(jac) = 9
        NodeVector nodes(3);
        nodes.add_item(0);
        nodes[0].point()[0] = 0.0;
        nodes[0].point()[1] = 1.0;
        nodes[0].point()[2] = 0.0;

        nodes.add_item(1);
        nodes[1].point()[0] = 2.0;
        nodes[1].point()[1] = 0.0;
        nodes[1].point()[2] = 0.0;

        nodes.add_item(2);
        nodes[2].point()[0] = 3.0;
        nodes[2].point()[1] = 4.0;
        nodes[2].point()[2] = 0.0;

        ElementVector el_vec(1);
        el_vec.add_item(0);

        RegionIdx reg;
        Element ele(2, NULL, reg);      //NULL - mesh pointer, empty RegionIdx

        ele.node = new Node * [ele.n_nodes()];
        for(int i =0; i < 3; i++) ele.node[i] = nodes(i);
        el_vec[0] = ele; // dangerous since Element has no deep copy constructor.

        ElementFullIter it( el_vec(0) );
        EXPECT_DOUBLE_EQ( 19.0 / 12.0 * 9.0 , integrate<2>( it ) );

        // projection methods
        MappingP1<2,3> mapping;
        arma::mat::fixed<3, 3> map = mapping.element_map(ele);
        EXPECT_ARMA_EQ( arma::mat("0 2 3; 1 -1 3; 0 0 0"), map);
        EXPECT_ARMA_EQ( arma::vec("0.6 0.2 0.2"), mapping.project_point( arma::vec("1.0 1.4 0.0"), map ) );
    }

}


class TestElementMapping : public Element {
public:
    TestElementMapping(std::vector<string> nodes_str)
    : Element()
    {
        std::vector<arma::vec3> nodes;
        for(auto str : nodes_str) nodes.push_back( arma::vec3(str));
        init(nodes.size()-1, nullptr, RegionIdx());
        unsigned int i=0;
        for(auto node : nodes)
            this->node[i++] = new Node(node[0], node[1], node[2]);
    }
};


TEST(ElementMapping, element_map) {
    Profiler::initialize();
    armadillo_setup();
    MappingP1<3,3> mapping;

    {
        TestElementMapping ele({ "0 0 0", "1 0 0", "0 1 0", "0 0 1"});
        arma::mat::fixed<3, 4> map = mapping.element_map(ele);
        EXPECT_ARMA_EQ( arma::mat("0 1 0 0; 0 0 1 0; 0 0 0 1"), map);
        EXPECT_ARMA_EQ( arma::vec("0.4 0.1 0.2 0.3"), mapping.project_point( arma::vec3("0.1 0.2 0.3"), map ) );
        EXPECT_ARMA_EQ( arma::vec("-0.5 0.5 0.5 0.5"), mapping.project_point( arma::vec3("0.5 0.5 0.5"), map ) );
    }

    {
        // trnaslated
        TestElementMapping ele({ "1 2 3", "2 2 3", "1 3 3", "1 2 4"});
        arma::mat::fixed<3, 4> map = mapping.element_map(ele);
        EXPECT_ARMA_EQ( arma::mat("1 1 0 0; 2 0 1 0; 3 0 0 1"), map);
        EXPECT_ARMA_EQ( arma::vec("0.4 0.1 0.2 0.3"), mapping.project_point( arma::vec3("1.1 2.2 3.3"), map ) );
        EXPECT_ARMA_EQ( arma::vec("-0.5 0.5 0.5 0.5"), mapping.project_point( arma::vec3("1.5 2.5 3.5"), map ) );
    }

    {
        // simplest cube element 7
        TestElementMapping ele({ "-1 -1 1", "1 1 -1", "-1 -1 -1", "1 -1 -1"});
        arma::mat::fixed<3, 4> map = mapping.element_map(ele);
        EXPECT_ARMA_EQ( arma::mat("-1 2 0 2; -1 2 0 0; 1 -2 -2 -2"), map);
        EXPECT_ARMA_EQ( arma::vec("0.25 0.25 0.25 0.25"), mapping.project_point( arma::vec3("0 -0.5 -0.5"), map ) );
        //EXPECT_ARMA_EQ( arma::vec("0.1 0.2 0.3 0.4"), mapping.project_point( arma::vec3("0.1 0.2 0.3"), map ) );
    }
}
