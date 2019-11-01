/*
 * field_formula_test.cpp
 *
 *  Created on: Jan 8, 2013
 *      Author: jb
 */


#define FEAL_OVERRIDE_ASSERTS

#include <flow_gtest.hh>
#include <mesh_constructor.hh>


#include "fields/field_constant.hh"
#include "fields/surface_depth.hh"
#include "fields/field_set.hh"
#include "fields/field.hh"
#include "input/input_type.hh"
#include "input/accessors.hh"
#include "input/reader_to_storage.hh"
#include "system/sys_profiler.hh"
#include "mesh/accessors.hh"
#include "mesh/range_wrapper.hh"



FLOW123D_FORCE_LINK_IN_PARENT(field_formula)
FLOW123D_FORCE_LINK_IN_PARENT(field_constant)


string input = R"YAML(
# formula on 2d
init_conc: !FieldFormula
  value: ["x", "x*y", "y+t"]
# formula on 2d
init_conc_unit_conversion: !FieldFormula
  value: ["x", "x*y", "y+t"]
  unit: "g*cm^-3"
# 3x3 tensor
conductivity_3d: !FieldFormula
  value: ["sin(x)+cos(y)","exp(x)+y^2", "base:=(x+y); base+base^2"]
# surface depth parameter
formula_with_depth: !FieldFormula
  value: ["d", "x", "y"]
  surface_region: ".top side"
# other field as parameter
field_as_param: !FieldFormula
  value: ["sigma_1", "sigma_2", "sigma_1+sigma_2"]
)YAML";

// Test input of FieldSet
const string eq_data_input = R"YAML(
- time: 0.0
  region: "BULK"
  sigma_1: !FieldConstant
    value: 1.1,
  sigma_2: !FieldConstant
    value: 2.2,
)YAML";


class SomeEquation {

public:
	class EqData : public FieldSet {
	public:
		EqData() {
		    *this += sigma_1.name("sigma_1")
		            .description("Transition coefficient between dimensions.")
		            .input_default("1.0")
		            .units( UnitSI::dimensionless() );
		    *this += sigma_2.name("sigma_2")
		            .description("Transition coefficient between dimensions.")
		            .input_default("1.0")
		            .units( UnitSI::dimensionless() );
		}

		// fields
	    Field<3, FieldValue<3>::Scalar > sigma_1;
	    Field<3, FieldValue<3>::Scalar > sigma_2;
	};

	SomeEquation() {
        //mesh_ = mesh_full_constructor("{mesh_file=\"fields/surface_reg.msh\"}");
	}

	~SomeEquation() {
		//delete mesh_;
	}

	FieldSet &eq_data() {
	    return data_;
	}

	//Mesh * mesh_;
	EqData data_;
	std::vector<string> component_names_;
};


TEST(FieldFormula, read_from_input) {
    typedef FieldAlgorithmBase<3, FieldValue<3>::TensorFixed > TensorField;
    typedef FieldAlgorithmBase<3, FieldValue<3>::VectorFixed > VectorField;

    Profiler::initialize();

    // setup FilePath directories
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");

    Input::Type::Record rec_type = Input::Type::Record("FieldFormulaTest","")
        .declare_key("conductivity_3d", TensorField::get_input_type_instance(), Input::Type::Default::obligatory(),"" )
        .declare_key("init_conc", VectorField::get_input_type_instance(), Input::Type::Default::obligatory(), "" )
        .declare_key("init_conc_unit_conversion", VectorField::get_input_type_instance(), Input::Type::Default::obligatory(), "" )
		.declare_key("formula_with_depth", VectorField::get_input_type_instance(), Input::Type::Default::obligatory(), "" )
		.declare_key("field_as_param", VectorField::get_input_type_instance(), Input::Type::Default::obligatory(), "" )
        .close();

    // read input string
    Input::ReaderToStorage reader( input, rec_type, Input::FileFormat::format_YAML );
    Input::Record in_rec=reader.get_root_interface<Input::Record>();

    Space<3>::Point point_1, point_2;
    point_1(0)=1.0; point_1(1)=2.0; point_1(2)=3.0;
    point_2(0)=2.0; point_2(1)=4.0; point_2(2)=6.0;
    ElementAccessor<3> elm;


    UnitSI unit_conc = UnitSI().kg().m(-3);
    FieldAlgoBaseInitData init_data_conc("init_conc", 3, unit_conc);
    auto conc=VectorField::function_factory(in_rec.val<Input::AbstractRecord>("init_conc"), init_data_conc);
    {
        arma::vec result;

        conc->set_time(0.0);
        result = conc->value( point_1, elm);
        EXPECT_DOUBLE_EQ( point_1(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( point_1(0)*point_1(1),    result[1]);
        EXPECT_DOUBLE_EQ( point_1(1),               result[2]);

        result = conc->value( point_2, elm);
        EXPECT_DOUBLE_EQ( point_2(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( point_2(0)*point_2(1),    result[1]);
        EXPECT_DOUBLE_EQ( point_2(1),               result[2]);

        conc->set_time(1.0);
        result = conc->value( point_1, elm);
        EXPECT_DOUBLE_EQ( point_1(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( point_1(0)*point_1(1),    result[1]);
        EXPECT_DOUBLE_EQ( point_1(1) +1.0,          result[2]);
    }

    auto conc_unit_conv=VectorField::function_factory(in_rec.val<Input::AbstractRecord>("init_conc_unit_conversion"), init_data_conc);
    {
        arma::vec result;
        double c = 1000.0; // multiplicative coefficient

        conc_unit_conv->set_time(0.0);
        result = conc_unit_conv->value( point_1, elm);
        EXPECT_DOUBLE_EQ( c*point_1(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( c*point_1(0)*point_1(1),    result[1]);
        EXPECT_DOUBLE_EQ( c*point_1(1),               result[2]);

        result = conc_unit_conv->value( point_2, elm);
        EXPECT_DOUBLE_EQ( c*point_2(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( c*point_2(0)*point_2(1),    result[1]);
        EXPECT_DOUBLE_EQ( c*point_2(1),               result[2]);

        conc_unit_conv->set_time(1.0);
        result = conc_unit_conv->value( point_1, elm);
        EXPECT_DOUBLE_EQ( c*point_1(0) ,              result[0]);
        EXPECT_DOUBLE_EQ( c*point_1(0)*point_1(1),    result[1]);
        EXPECT_DOUBLE_EQ( c*(point_1(1) +1.0),          result[2]);
    }

    FieldAlgoBaseInitData init_data_conductivity("conductivity_3d", 0, UnitSI::dimensionless());
    auto cond=TensorField::function_factory(in_rec.val<Input::AbstractRecord>("conductivity_3d"), init_data_conductivity);
    cond->set_time(0.0);
    {
        arma::mat::fixed<3,3> result;
        double x,y,base;

        result = cond->value( point_1, elm);
        x=point_1(0); y=point_1(1); base=(x+y);
        EXPECT_DOUBLE_EQ( sin(x)+cos(y),              result.at(0,0));
        EXPECT_DOUBLE_EQ( 0.0,                      result.at(0,1));
        EXPECT_DOUBLE_EQ( 0.0,                      result.at(0,2));

        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(1,0));
        EXPECT_DOUBLE_EQ( exp(x)+y*y,               result.at(1,1));
        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(1,2));

        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(2,0));
        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(2,1));
        EXPECT_DOUBLE_EQ( base+base*base,               result.at(2,2));

        result = cond->value( point_2, elm);
        x=point_2(0); y=point_2(1); base=(x+y);
        EXPECT_DOUBLE_EQ( sin(x)+cos(y),              result.at(0,0));
        EXPECT_DOUBLE_EQ( 0.0,                      result.at(0,1));
        EXPECT_DOUBLE_EQ( 0.0,                      result.at(0,2));

        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(1,0));
        EXPECT_DOUBLE_EQ( exp(x)+y*y,               result.at(1,1));
        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(1,2));

        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(2,0));
        EXPECT_DOUBLE_EQ( 0.0 ,                     result.at(2,1));
        EXPECT_DOUBLE_EQ( base+base*base,               result.at(2,2));

    }
    FieldAlgoBaseInitData init_data_depth("formula_with_depth", 3, UnitSI::dimensionless());
    auto depth=VectorField::function_factory(in_rec.val<Input::AbstractRecord>("formula_with_depth"), init_data_depth);
    {
    	std::string mesh_in_string = "{mesh_file=\"fields/surface_reg.msh\"}";
    	Mesh * mesh = mesh_constructor(mesh_in_string);
        auto reader = reader_constructor(mesh_in_string);
    	reader->read_physical_names(mesh);
    	reader->read_raw_mesh(mesh);

    	depth->set_mesh( const_cast<const Mesh *>(mesh), true );
    	depth->set_time(0.0);

        arma::vec3 result;
        Space<3>::Point point;
        for( auto elem : mesh->elements_range() ) {
        	point = elem.centre();
        	result = depth->value(point, elem);
        	EXPECT_DOUBLE_EQ(result(0), 1-point(2));
        	EXPECT_DOUBLE_EQ(result(1), point(0));
        	EXPECT_DOUBLE_EQ(result(2), point(1));
        }
    }
    FieldAlgoBaseInitData init_data_param("field_as_param", 3, UnitSI::dimensionless());
    auto field_param=VectorField::function_factory(in_rec.val<Input::AbstractRecord>("field_as_param"), init_data_param);
    {
        SomeEquation eq;
        field_param->set_dependency(eq.eq_data());
        field_param->set_time(0.0);

        arma::vec3 result;
        result = field_param->value( point_1, elm);
        std::cout << result;
    	//EXPECT_DOUBLE_EQ(result(0), 1.1);
    	//EXPECT_DOUBLE_EQ(result(1), 2.2);
    	//EXPECT_DOUBLE_EQ(result(2), 3.3);
    }
}


string set_time_input = R"INPUT(
[ 
      { TYPE="FieldFormula",  value=["x", "x*y", "y+t"] },
      { TYPE="FieldFormula",  value=["x", "x*y", "y"] },
      { TYPE="FieldFormula",  value=["x+t", "x*y+t", "y+t"] },
      { TYPE="FieldFormula",  value=["x", "x*y", "y"] }
]

)INPUT";


TEST(FieldFormula, set_time) {
    typedef FieldAlgorithmBase<3, FieldValue<3>::VectorFixed > VectorField;

    Profiler::initialize();

    // setup FilePath directories
    FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");

    Input::Type::Array  input_type(VectorField::get_input_type_instance());
    input_type.finish();

    // read input string
    Input::ReaderToStorage reader( set_time_input, input_type, Input::FileFormat::format_JSON );
    Input::Array in_array=reader.get_root_interface<Input::Array>();

    auto it = in_array.begin<Input::AbstractRecord>();
    FieldAlgoBaseInitData init_data("formula", 3, UnitSI::dimensionless());

    {
        auto field=VectorField::function_factory(*it, init_data);
        EXPECT_TRUE( field->set_time(1.0) );
        EXPECT_TRUE( field->set_time(2.0) );
    }
    ++it;

    {
        auto field=VectorField::function_factory(*it, init_data);
        EXPECT_TRUE( field->set_time(3.0) );
        EXPECT_FALSE( field->set_time(4.0) );
    }
    ++it;

    {
        auto field=VectorField::function_factory(*it, init_data);
        EXPECT_TRUE( field->set_time(1.5) );
        EXPECT_TRUE( field->set_time(2.5) );
    }
    ++it;

    {
        auto field=VectorField::function_factory(*it, init_data);
        EXPECT_TRUE( field->set_time(0.0) );
        EXPECT_FALSE( field->set_time(2.0) );
    }

}


TEST(SurfaceDepth, base_test) {
    Profiler::initialize();
	FilePath::set_io_dirs(".",UNIT_TESTS_SRC_DIR,"",".");

	std::string mesh_in_string = "{mesh_file=\"fields/surface_reg.msh\"}";
	Mesh * mesh = mesh_constructor(mesh_in_string);
    auto reader = reader_constructor(mesh_in_string);
	reader->read_physical_names(mesh);
	reader->read_raw_mesh(mesh);

	SurfaceDepth sd(mesh, ".top side", "0 0 1");
	EXPECT_DOUBLE_EQ( sd.compute_distance( arma::vec3("1 0.5 -0.9") ), 1.9 );
	EXPECT_DOUBLE_EQ( sd.compute_distance( arma::vec3("-1 0.5 0.9") ), 0.1 );

	delete mesh;
}
