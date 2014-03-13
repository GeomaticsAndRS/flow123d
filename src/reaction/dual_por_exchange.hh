/** @brief Class Dual_por_exchange implements the model of dual porosity.
 *
 * It can be part of the transport model and it computes the concentrations of substances both in 
 * mobile and immobile zone. This model can also work above the sorption model - the sorbed concentration
 * is then computed both from mobile and immobile concentrations. Linear reactions can be define 
 * also in both zones.
 *
 */
#ifndef DUAL_POROSITY
#define DUAL_POROSITY

#include <vector>
#include <input/input_type.hh>

#include "fields/field_base.hh"
#include "fields/field_set.hh"
#include "./reaction/reaction.hh"

/// TODO: incorporate index mapping for substances indices

class Mesh;
class Distribution;
class SorptionBase;

class Dual_por_exchange:  public Reaction
{
public:
  /**
   * Static variable for new input data types input
   */
  static Input::Type::Record input_type;

	class EqData : public FieldSet // should be written in class Sorption
  {
  public:

    /// Collect all fields
    EqData();

    Field<3, FieldValue<3>::Vector > alpha;            ///< Mass transfer coefficients between mobile and immobile pores.
    Field<3, FieldValue<3>::Scalar > immob_porosity;    ///< Immobile porosity field.
    
    Field<3, FieldValue<3>::Vector> init_conc_immobile; ///< Initial concentrations in the immobile zone. 

    Field<3, FieldValue<3>::Scalar > porosity; ///< Porosity field.
  };

  Dual_por_exchange(Mesh &init_mesh, Input::Record in_rec, vector<string> &names);
  /**
   * Destructor.
   */
  ~Dual_por_exchange(void);
                
  /**
   * Updates the solution according to the dual porosity model.
   */
  void update_solution(void) override;
  
  void set_concentration_matrix(double **ConcentrationMatrix, Distribution *conc_distr, int *el_4_loc_) override;
  /**
   *
   */
  inline void set_porosity(Field<3, FieldValue<3>::Scalar > &por_m)
    { data_.set_field(data_.porosity.name(),por_m); };
  
  /// Initialize from input interface.
  void init_from_input(Input::Record in_rec) override;
  
  double **compute_reaction(double **concentrations, int loc_el) override;
  
protected:
  /**
   * This method disables to use constructor without parameters.
   */
  Dual_por_exchange();

  /**
   * Pointer to thwodimensional array[species][elements] containing concentrations either in immobile.
   */
  double **immob_concentration_matrix;

  /**
   *
   */
  EqData data_;
  
  
  Reaction *reaction_mob;       //< Reaction running in mobile zone
  Reaction *reaction_immob;     //< Reaction running in immobile zone
  
  /** Minimal time for which the analytical solution of dual porosity concentrations are evaluated.
   * Else it is replaced with simple forward difference approximation.
   */
  static const double min_dt;
  
};

#endif  //DUAL_POROSITY
