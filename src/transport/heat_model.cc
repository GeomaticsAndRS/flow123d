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
 * @file    heat_model.cc
 * @brief   Discontinuous Galerkin method for equation of transport with dispersion.
 * @author  Jan Stebel
 */

#include "input/input_type.hh"
#include "mesh/mesh.h"
#include "mesh/accessors.hh"
//#include "transport/transport_operator_splitting.hh"
#include "heat_model.hh"
#include "tools/unit_si.hh"
#include "coupling/balance.hh"
#include "fields/field_model.hh"



using namespace std;
using namespace Input::Type;


/*******************************************************************************
 * Functors of FieldModels
 */
using Sclr = double;
using Vect = arma::vec3;
using Tens = arma::mat33;

// Functor computing velocity norm
Sclr fn_heat_v_norm(Vect vel) {
	return arma::norm(vel, 2);
}

/**
 * Functor computing mass matrix coefficients:
 * cross_section * (porosity*fluid_density*fluid_heat_capacity + (1.-porosity)*solid_density*solid_heat_capacity)
 */
Sclr fn_heat_mass_matrix(Sclr csec, Sclr por, Sclr f_rho, Sclr f_c, Sclr s_rho, Sclr s_c) {
    return csec * (por*f_rho*f_c + (1.-por)*s_rho*s_c);
}

/**
 * Functor computing sources density output:
 * cross_section * (porosity*fluid_thermal_source + (1-porosity)*solid_thermal_source)
 */
Sclr fn_heat_sources_dens(Sclr csec, Sclr por, Sclr f_source, Sclr s_source) {
    return csec * (por * f_source + (1.-por) * s_source);
}

/**
 * Functor computing sources sigma output:
 * cross_section * (porosity*fluid_density*fluid_heat_capacity*fluid_heat_exchange_rate + (1-porosity)*solid_density*solid_heat_capacity*solid_thermal_source)
 */
Sclr fn_heat_sources_sigma(Sclr csec, Sclr por, Sclr f_rho, Sclr f_cap, Sclr f_sigma, Sclr s_rho, Sclr s_cap, Sclr s_sigma) {
    return csec * (por * f_rho * f_cap * f_sigma + (1.-por) * s_rho * s_cap * s_sigma);
}

/**
 * Functor computing sources concentration output for positive heat_sources_sigma (otherwise return 0):
 * cross_section * (porosity*fluid_density*fluid_heat_capacity*fluid_heat_exchange_rate*fluid_ref_temperature + (1-porosity)*solid_density*solid_heat_capacity*solid_heat_exchange_rate*solid_ref_temperature)
 */
Sclr fn_heat_sources_conc(Sclr csec, Sclr por, Sclr f_rho, Sclr f_cap, Sclr f_sigma, Sclr f_temp, Sclr s_rho, Sclr s_cap, Sclr s_sigma, Sclr s_temp, Sclr sigma) {
    if (fabs(sigma) > numeric_limits<double>::epsilon())
        return csec * (por * f_rho * f_cap * f_sigma * f_temp + (1.-por) * s_rho * s_cap * s_sigma * s_temp);
    else {
    	return 0;
    }
}

/**
 * Functor computing advection coefficient
 * velocity * fluid_density * fluid_heat_capacity
 */
Vect fn_heat_ad_coef(Sclr f_rho, Sclr f_cap, Vect velocity) {
    return velocity * f_rho * f_cap;
}

/**
 * Functor computing diffusion coefficient (see notes in function)
 */
Tens fn_heat_diff_coef(Vect velocity, Sclr v_norm, Sclr f_rho, Sclr disp_l, Sclr disp_t, Sclr f_cond, Sclr s_cond, Sclr c_sec, Sclr por) {
	// result
	Tens dif_coef;

	// dispersive part of thermal diffusion
	// Note that the velocity vector is in fact the Darcian flux,
	// so to obtain |v| we have to divide vnorm by porosity and cross_section.
	if ( fabs(v_norm) > 0 )
		for (int i=0; i<3; i++)
			for (int j=0; j<3; j++)
				dif_coef(i,j) = ( (velocity(i)/v_norm) * (velocity(j)/v_norm) * (disp_l-disp_t) + disp_t*(i==j?1:0))*v_norm*f_rho*f_cond;
	else
		dif_coef.zeros();

	// conductive part of thermal diffusion
	dif_coef += c_sec * (por*f_cond + (1.-por)*s_cond) * arma::eye(3,3);
    return dif_coef;
}








const Selection & HeatTransferModel::ModelEqData::get_bc_type_selection() {
	return Selection("Heat_BC_Type", "Types of boundary conditions for heat transfer model.")
            .add_value(bc_inflow, "inflow",
          		  "Default heat transfer boundary condition.\n"
          		  "On water inflow (($(q_w \\le 0)$)), total energy flux is given by the reference temperature 'bc_temperature'. "
          		  "On water outflow we prescribe zero diffusive flux, "
          		  "i.e. the energy flows out only due to advection.")
            .add_value(bc_dirichlet, "dirichlet",
          		  "Dirichlet boundary condition (($T = T_D $)).\n"
          		  "The prescribed temperature (($T_D$)) is specified by the field 'bc_temperature'.")
            .add_value(bc_total_flux, "total_flux",
          		  "Total energy flux boundary condition.\n"
          		  "The prescribed incoming total flux can have the general form (($\\delta(f_N+\\sigma_R(T_R-T) )$)), "
          		  "where the absolute flux (($f_N$)) is specified by the field 'bc_flux', "
          		  "the transition parameter (($\\sigma_R$)) by 'bc_robin_sigma', "
          		  "and the reference temperature (($T_R$)) by 'bc_temperature'.")
            .add_value(bc_diffusive_flux, "diffusive_flux",
          		  "Diffusive flux boundary condition.\n"
          		  "The prescribed incoming energy flux due to diffusion can have the general form (($\\delta(f_N+\\sigma_R(T_R-T) )$)), "
          		  "where the absolute flux (($f_N$)) is specified by the field 'bc_flux', "
          		  "the transition parameter (($\\sigma_R$)) by 'bc_robin_sigma', "
          		  "and the reference temperature (($T_R$)) by 'bc_temperature'.")
			  .close();
}


HeatTransferModel::ModelEqData::ModelEqData()
{
    *this+=bc_type
            .name("bc_type")
            .description(
            "Type of boundary condition.")
            .units( UnitSI::dimensionless() )
            .input_default("\"inflow\"")
            .input_selection( get_bc_type_selection() )
            .flags_add(FieldFlag::in_rhs & FieldFlag::in_main_matrix);
    *this+=bc_dirichlet_value
            .name("bc_temperature")
            .description("Boundary value of temperature.")
            .units( UnitSI().K() )
            .input_default("0.0")
            .flags_add(in_rhs);
	*this+=bc_flux
	        .disable_where(bc_type, { bc_dirichlet, bc_inflow })
			.name("bc_flux")
			.description("Flux in Neumann boundary condition.")
			.units( UnitSI().kg().m().s(-1).md() )
			.input_default("0.0")
			.flags_add(FieldFlag::in_rhs);
	*this+=bc_robin_sigma
	        .disable_where(bc_type, { bc_dirichlet, bc_inflow })
			.name("bc_robin_sigma")
			.description("Conductivity coefficient in Robin boundary condition.")
			.units( UnitSI().m(4).s(-1).md() )
			.input_default("0.0")
			.flags_add(FieldFlag::in_rhs & FieldFlag::in_main_matrix);

    *this+=init_temperature
            .name("init_temperature")
            .description("Initial temperature.")
            .units( UnitSI().K() )
            .input_default("0.0");

    *this+=porosity
            .name("porosity")
            .description("Porosity.")
            .units( UnitSI::dimensionless() )
            .input_default("1.0")
            .flags_add(in_main_matrix & in_time_term)
			.set_limits(0.0);

    *this+=water_content
            .name("water_content")
            .units( UnitSI::dimensionless() )
            .input_default("1.0")
            .flags_add(input_copy & in_main_matrix & in_time_term);

    *this+=fluid_density
            .name("fluid_density")
            .description("Density of fluid.")
            .units( UnitSI().kg().m(-3) )
            .input_default("1000")
            .flags_add(in_main_matrix & in_time_term);

    *this+=fluid_heat_capacity
            .name("fluid_heat_capacity")
            .description("Heat capacity of fluid.")
            .units( UnitSI::J() * UnitSI().kg(-1).K(-1) )
            .flags_add(in_main_matrix & in_time_term);

    *this+=fluid_heat_conductivity
            .name("fluid_heat_conductivity")
            .description("Heat conductivity of fluid.")
            .units( UnitSI::W() * UnitSI().m(-1).K(-1) )
            .flags_add(in_main_matrix)
			.set_limits(0.0);


    *this+=solid_density
            .name("solid_density")
            .description("Density of solid (rock).")
            .units( UnitSI().kg().m(-3) )
            .flags_add(in_time_term);

    *this+=solid_heat_capacity
            .name("solid_heat_capacity")
            .description("Heat capacity of solid (rock).")
            .units( UnitSI::J() * UnitSI().kg(-1).K(-1) )
            .flags_add(in_time_term);

    *this+=solid_heat_conductivity
            .name("solid_heat_conductivity")
            .description("Heat conductivity of solid (rock).")
            .units( UnitSI::W() * UnitSI().m(-1).K(-1) )
            .flags_add(in_main_matrix)
			.set_limits(0.0);

    *this+=disp_l
            .name("disp_l")
            .description("Longitudinal heat dispersivity in fluid.")
            .units( UnitSI().m() )
            .input_default("0.0")
            .flags_add(in_main_matrix);

    *this+=disp_t
            .name("disp_t")
            .description("Transverse heat dispersivity in fluid.")
            .units( UnitSI().m() )
            .input_default("0.0")
            .flags_add(in_main_matrix);

    *this+=fluid_thermal_source
            .name("fluid_thermal_source")
            .description("Density of thermal source in fluid.")
            .units( UnitSI::W() * UnitSI().m(-3) )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=solid_thermal_source
            .name("solid_thermal_source")
            .description("Density of thermal source in solid.")
            .units( UnitSI::W() * UnitSI().m(-3) )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=fluid_heat_exchange_rate
            .name("fluid_heat_exchange_rate")
            .description("Heat exchange rate of source in fluid.")
            .units( UnitSI().s(-1) )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=solid_heat_exchange_rate
            .name("solid_heat_exchange_rate")
            .description("Heat exchange rate of source in solid.")
            .units( UnitSI().s(-1) )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=fluid_ref_temperature
            .name("fluid_ref_temperature")
            .description("Reference temperature of source in fluid.")
            .units( UnitSI().K() )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=solid_ref_temperature
            .name("solid_ref_temperature")
            .description("Reference temperature in solid.")
            .units( UnitSI().K() )
            .input_default("0.0")
            .flags_add(in_rhs);

    *this+=cross_section
            .name("cross_section")
            .units( UnitSI().m(3).md() )
            .flags(input_copy & in_time_term & in_main_matrix);

    *this+=output_field
            .name("temperature")
            .description("Temperature solution.")
            .units( UnitSI().K() )
            .flags(equation_result);

    *this += velocity.name("velocity")
            .description("Velocity field given from Flow equation.")
            .input_default("0.0")
            .units( UnitSI().m().s(-1) );


	// initiaization of FieldModels
    *this += v_norm.name("v_norm")
            .description("Velocity norm field.")
            .input_default("0.0")
            .units( UnitSI().m().s(-1) );

    *this += mass_matrix_coef.name("mass_matrix_coef")
            .description("Matrix coefficients computed by model in mass assemblation.")
            .input_default("0.0")
            .units( UnitSI().m(3).md() );

    *this += retardation_coef.name("retardation_coef")
            .description("Retardation coefficients computed by model in mass assemblation.")
            .input_default("0.0")
            .units( UnitSI().m(3).md() );

    *this += sources_conc_out.name("sources_conc_out")
            .description("Concentration sources output.")
            .input_default("0.0")
            .units( UnitSI().kg().m(-3) );

    *this += sources_density_out.name("sources_density_out")
            .description("Concentration sources output - density of substance source, only positive part is used..")
            .input_default("0.0")
            .units( UnitSI().kg().s(-1).md() );

    *this += sources_sigma_out.name("sources_sigma_out")
            .description("Concentration sources - Robin type, in_flux = sources_sigma * (sources_conc - mobile_conc).")
            .input_default("0.0")
            .units( UnitSI().s(-1).m(3).md() );

    *this += advection_coef.name("advection_coef")
            .description("Advection coefficients model.")
            .input_default("0.0")
            .units( UnitSI().m().s(-1) );

    *this += diffusion_coef.name("diffusion_coef")
            .description("Diffusion coefficients model.")
            .input_default("0.0")
            .units( UnitSI().m(2).s(-1) );
}



IT::Record HeatTransferModel::get_input_type(const string &implementation, const string &description)
{
	return IT::Record(
				std::string(ModelEqData::name()) + "_" + implementation,
				description + " for heat transfer.")
			.derive_from(AdvectionProcessBase::get_input_type())
			.copy_keys(EquationBase::record_template())
			.declare_key("balance", Balance::get_input_type(), Default("{}"),
					"Settings for computing balance.")
			.declare_key("output_stream", OutputTime::get_input_type(), Default("{}"),
					"Parameters of output stream.");
}


IT::Selection HeatTransferModel::ModelEqData::get_output_selection()
{
    // Return empty selection just to provide model specific selection name and description.
    // The fields are added by TransportDG using an auxiliary selection.
	return IT::Selection(
				std::string(ModelEqData::name()) + "_DG_output_fields",
				"Selection of output fields for Heat Transfer DG model.");
}


HeatTransferModel::HeatTransferModel(Mesh &mesh, const Input::Record in_rec) :
		AdvectionProcessBase(mesh, in_rec),
		flux_changed(true)
{
	time_ = new TimeGovernor(in_rec.val<Input::Record>("time"));
	ASSERT( time_->is_default() == false ).error("Missing key 'time' in Heat_AdvectionDiffusion_DG.");
	substances_.initialize({""});

    output_stream_ = OutputTime::create_output_stream("heat", in_rec.val<Input::Record>("output_stream"), time().get_unit_string());
    //output_stream_->add_admissible_field_names(in_rec.val<Input::Array>("output_fields"));

    balance_ = std::make_shared<Balance>("energy", mesh_);
    balance_->init_from_input(in_rec.val<Input::Record>("balance"), *time_);
    // initialization of balance object
    subst_idx = {balance_->add_quantity("energy")};
    balance_->units(UnitSI().m(2).kg().s(-2));
}


void HeatTransferModel::output_data()
{
	output_stream_->write_time_frame();
}


/*void HeatTransferModel::compute_mass_matrix_coefficient(const Armor::array &point_list,
		const ElementAccessor<3> &ele_acc,
		std::vector<double> &mm_coef)
{
	vector<double> elem_csec(point_list.size()),
			por(point_list.size()),
			f_rho(point_list.size()),
			s_rho(point_list.size()),
			f_c(point_list.size()),
			s_c(point_list.size());

	data().cross_section.value_list(point_list, ele_acc, elem_csec);
	data().porosity.value_list(point_list, ele_acc, por);
	data().fluid_density.value_list(point_list, ele_acc, f_rho);
	data().fluid_heat_capacity.value_list(point_list, ele_acc, f_c);
	data().solid_density.value_list(point_list, ele_acc, s_rho);
	data().solid_heat_capacity.value_list(point_list, ele_acc, s_c);

	for (unsigned int i=0; i<point_list.size(); i++)
		mm_coef[i] = elem_csec[i]*(por[i]*f_rho[i]*f_c[i] + (1.-por[i])*s_rho[i]*s_c[i]);
}*/


void HeatTransferModel::compute_advection_diffusion_coefficients(const Armor::array &point_list,
		const std::vector<arma::vec3> &velocity,
		const ElementAccessor<3> &ele_acc,
		std::vector<std::vector<arma::vec3> > &ad_coef,
		std::vector<std::vector<arma::mat33> > &dif_coef)
{
	const unsigned int qsize = point_list.size();
	std::vector<double> f_rho(qsize), f_cap(qsize), f_cond(qsize),
			s_cond(qsize), por(qsize), csection(qsize), disp_l(qsize), disp_t(qsize);

	data().fluid_density.value_list(point_list, ele_acc, f_rho);
	data().fluid_heat_capacity.value_list(point_list, ele_acc, f_cap);
	data().fluid_heat_conductivity.value_list(point_list, ele_acc, f_cond);
	data().solid_heat_conductivity.value_list(point_list, ele_acc, s_cond);
	data().disp_l.value_list(point_list, ele_acc, disp_l);
	data().disp_t.value_list(point_list, ele_acc, disp_t);
	data().porosity.value_list(point_list, ele_acc, por);
	data().cross_section.value_list(point_list, ele_acc, csection);

	for (unsigned int k=0; k<qsize; k++) {
		ad_coef[0][k] = velocity[k]*f_rho[k]*f_cap[k];

		// dispersive part of thermal diffusion
		// Note that the velocity vector is in fact the Darcian flux,
		// so to obtain |v| we have to divide vnorm by porosity and cross_section.
		double vnorm = arma::norm(velocity[k], 2);
		if (fabs(vnorm) > 0)
			for (int i=0; i<3; i++)
				for (int j=0; j<3; j++)
					dif_coef[0][k](i,j) = ((velocity[k][i]/vnorm)*(velocity[k][j]/vnorm)*(disp_l[k]-disp_t[k]) + disp_t[k]*(i==j?1:0))
											*vnorm*f_rho[k]*f_cond[k];
		else
			dif_coef[0][k].zeros();

		// conductive part of thermal diffusion
		dif_coef[0][k] += csection[k]*(por[k]*f_cond[k] + (1.-por[k])*s_cond[k])*arma::eye(3,3);
	}
}


/*void HeatTransferModel::compute_init_cond(const Armor::array &point_list,
		const ElementAccessor<3> &ele_acc,
		std::vector<std::vector<double> > &init_values)
{
	data().init_temperature.value_list(point_list, ele_acc, init_values[0]);
}*/


void HeatTransferModel::get_bc_type(const ElementAccessor<3> &ele_acc,
			arma::uvec &bc_types)
{
	// Currently the bc types for HeatTransfer are numbered in the same way as in TransportDG.
	// In general we should use some map here.
	bc_types = { data().bc_type.value(ele_acc.centre(), ele_acc) }; //TODO change bc_type to MultiField
}


void HeatTransferModel::get_flux_bc_data(unsigned int index,
        const Armor::array &point_list,
		const ElementAccessor<3> &ele_acc,
		std::vector< double > &bc_flux,
		std::vector< double > &bc_sigma,
		std::vector< double > &bc_ref_value)
{
	data().bc_flux.value_list(point_list, ele_acc, bc_flux);
	data().bc_robin_sigma.value_list(point_list, ele_acc, bc_sigma);
	data().bc_dirichlet_value[index].value_list(point_list, ele_acc, bc_ref_value);
	
	// Change sign in bc_flux since internally we work with outgoing fluxes.
	for (auto f : bc_flux) f = -f;
}

void HeatTransferModel::get_flux_bc_sigma(FMT_UNUSED unsigned int index,
        const Armor::array &point_list,
		const ElementAccessor<3> &ele_acc,
		std::vector< double > &bc_sigma)
{
	data().bc_robin_sigma.value_list(point_list, ele_acc, bc_sigma);
}


/*void HeatTransferModel::compute_source_coefficients(const Armor::array &point_list,
			const ElementAccessor<3> &ele_acc,
			std::vector<std::vector<double> > &sources_value,
			std::vector<std::vector<double> > &sources_density,
			std::vector<std::vector<double> > &sources_sigma)
{
	const unsigned int qsize = point_list.size();
	std::vector<double> por(qsize), csection(qsize), f_rho(qsize), s_rho(qsize), f_cap(qsize), s_cap(qsize),
			f_source(qsize), s_source(qsize), f_sigma(qsize), s_sigma(qsize), f_temp(qsize), s_temp(qsize);
	data().porosity.value_list(point_list, ele_acc, por);
	data().cross_section.value_list(point_list, ele_acc, csection);
	data().fluid_density.value_list(point_list, ele_acc, f_rho);
	data().solid_density.value_list(point_list, ele_acc, s_rho);
	data().fluid_heat_capacity.value_list(point_list, ele_acc, f_cap);
	data().solid_heat_capacity.value_list(point_list, ele_acc, s_cap);
	data().fluid_thermal_source.value_list(point_list, ele_acc, f_source);
	data().solid_thermal_source.value_list(point_list, ele_acc, s_source);
	data().fluid_heat_exchange_rate.value_list(point_list, ele_acc, f_sigma);
	data().solid_heat_exchange_rate.value_list(point_list, ele_acc, s_sigma);
	data().fluid_ref_temperature.value_list(point_list, ele_acc, f_temp);
	data().solid_ref_temperature.value_list(point_list, ele_acc, s_temp);

    sources_density[0].resize(point_list.size());
    sources_sigma[0].resize(point_list.size());
    sources_value[0].resize(point_list.size());
	for (unsigned int k=0; k<point_list.size(); k++)
	{
		sources_density[0][k] = csection[k]*(por[k]*f_source[k] + (1.-por[k])*s_source[k]);
		sources_sigma[0][k] = csection[k]*(por[k]*f_rho[k]*f_cap[k]*f_sigma[k] + (1.-por[k])*s_rho[k]*s_cap[k]*s_sigma[k]);
		if (fabs(sources_sigma[0][k]) > numeric_limits<double>::epsilon())
			sources_value[0][k] = csection[k]*(por[k]*f_rho[k]*f_cap[k]*f_sigma[k]*f_temp[k]
		                   + (1.-por[k])*s_rho[k]*s_cap[k]*s_sigma[k]*s_temp[k])/sources_sigma[0][k];
		else
			sources_value[0][k] = 0;
	}
}*/


/*void HeatTransferModel::compute_sources_sigma(const Armor::array &point_list,
			const ElementAccessor<3> &ele_acc,
			std::vector<std::vector<double> > &sources_sigma)
{
	const unsigned int qsize = point_list.size();
	std::vector<double> por(qsize), csection(qsize), f_rho(qsize), s_rho(qsize), f_cap(qsize), s_cap(qsize),
			f_source(qsize), s_source(qsize), f_sigma(qsize), s_sigma(qsize), f_temp(qsize), s_temp(qsize);
	data().porosity.value_list(point_list, ele_acc, por);
	data().cross_section.value_list(point_list, ele_acc, csection);
	data().fluid_density.value_list(point_list, ele_acc, f_rho);
	data().solid_density.value_list(point_list, ele_acc, s_rho);
	data().fluid_heat_capacity.value_list(point_list, ele_acc, f_cap);
	data().solid_heat_capacity.value_list(point_list, ele_acc, s_cap);
	data().fluid_heat_exchange_rate.value_list(point_list, ele_acc, f_sigma);
	data().solid_heat_exchange_rate.value_list(point_list, ele_acc, s_sigma);
    sources_sigma[0].resize(point_list.size());
	for (unsigned int k=0; k<point_list.size(); k++)
	{
		sources_sigma[0][k] = csection[k]*(por[k]*f_rho[k]*f_cap[k]*f_sigma[k] + (1.-por[k])*s_rho[k]*s_cap[k]*s_sigma[k]);
	}
}*/


void HeatTransferModel::initialize()
{
    // initialize multifield components
	// empty for now

    // create FieldModels
    auto v_norm_ptr = Model<3, FieldValue<3>::Scalar>::create(fn_heat_v_norm, data().velocity);
    data().v_norm.set_field(mesh_->region_db().get_region_set("ALL"), v_norm_ptr);

    auto mass_matrix_coef_ptr = Model<3, FieldValue<3>::Scalar>::create(fn_heat_mass_matrix, data().cross_section,
            data().porosity, data().fluid_density, data().fluid_heat_capacity, data().solid_density, data().solid_heat_capacity);
    data().mass_matrix_coef.set_field(mesh_->region_db().get_region_set("ALL"), mass_matrix_coef_ptr);

    std::vector<typename Field<3, FieldValue<3>::Scalar>::FieldBasePtr> retardation_coef_ptr;
    retardation_coef_ptr.push_back( std::make_shared< FieldConstant<3, FieldValue<3>::Scalar> >() ); // Fix size of substances == 1, with const value == 0
    data().retardation_coef.set_fields(mesh_->region_db().get_region_set("ALL"), retardation_coef_ptr);

    auto sources_dens_ptr = Model<3, FieldValue<3>::Scalar>::create_multi(fn_heat_sources_dens, data().cross_section, data().porosity,
            data().fluid_thermal_source, data().solid_thermal_source);
    data().sources_density_out.set_fields(mesh_->region_db().get_region_set("ALL"), sources_dens_ptr);

    auto sources_sigma_ptr = Model<3, FieldValue<3>::Scalar>::create_multi(fn_heat_sources_sigma, data().cross_section, data().porosity,
            data().fluid_density, data().fluid_heat_capacity, data().fluid_heat_exchange_rate, data().solid_density,
            data().solid_heat_capacity, data().solid_heat_exchange_rate);
    data().sources_sigma_out.set_fields(mesh_->region_db().get_region_set("ALL"), sources_sigma_ptr);

    auto sources_conc_ptr = Model<3, FieldValue<3>::Scalar>::create_multi(fn_heat_sources_conc, data().cross_section, data().porosity,
            data().fluid_density, data().fluid_heat_capacity, data().fluid_heat_exchange_rate, data().fluid_ref_temperature, data().solid_density,
            data().solid_heat_capacity, data().solid_heat_exchange_rate, data().solid_ref_temperature, data().sources_sigma_out);
    data().sources_conc_out.set_fields(mesh_->region_db().get_region_set("ALL"), sources_conc_ptr);

    auto ad_coef_ptr = Model<3, FieldValue<3>::VectorFixed>::create(fn_heat_ad_coef, data().fluid_density, data().fluid_heat_capacity, data().velocity);
    std::vector<typename Field<3, FieldValue<3>::VectorFixed>::FieldBasePtr> ad_coef_ptr_vec;
    ad_coef_ptr_vec.push_back(ad_coef_ptr);
    data().advection_coef.set_fields(mesh_->region_db().get_region_set("ALL"), ad_coef_ptr_vec);

    auto diff_coef_ptr = Model<3, FieldValue<3>::TensorFixed>::create(fn_heat_diff_coef, data().velocity, data().v_norm, data().fluid_density,
            data().disp_l, data().disp_t, data().fluid_heat_conductivity, data().solid_heat_conductivity, data().cross_section, data().porosity);
    std::vector<typename Field<3, FieldValue<3>::TensorFixed>::FieldBasePtr> diff_coef_ptr_vec;
    diff_coef_ptr_vec.push_back(diff_coef_ptr);
    data().diffusion_coef.set_fields(mesh_->region_db().get_region_set("ALL"), diff_coef_ptr_vec);
}


HeatTransferModel::~HeatTransferModel()
{}




