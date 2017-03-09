#ifdef WIN32
#define _USE_MATH_DEFINES //needed for M_PI
#endif
#include <math.h>
#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_INIT scania_driveline_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"


#if defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif

#define SQ(x)  ( x ) * ( x )
#define min(x,y)  ( ( x ) < ( y ) )?  x : y
#define max(x,y)  ( ( x ) > ( y ) )?  x : y



#define inputs (s->md)
#define outputs (&s->md)
int  fcn( double t, const double * x, double *dxdt, void * params){

  state_t *s = (state_t*)params;
  /// copy the struct: we don't write to this
  /* ins   inputs  =   ( ( everything * ) params)->inputs; */
  /* /// pointer access needed for outputs */
  /* outs *outputs = & ( ( everything * ) params)->outputs; */

  /// real state is in x
  /// This makes only temporary changes.
  /// TODO: should we remove these variables from the input struct?
  inputs.w_inShaftNeutral = x[0];
  inputs.w_wheel          = x[1];

  outputs->w_inShaft = inputs.w_inShaftOld;

  // torque from input springs
  double tq_inputWheel = inputs.w_wheel_in * inputs.k1 + inputs.f_wheel_in;
  double tq_inputShaft = inputs.w_shaft_in * inputs.k2 + inputs.f_shaft_in;

  double tq_retWheel = inputs.tq_retarder * inputs.final_gear_ratio;

  // This is the sum of external forces acting on the vehicle:
  // wind + roll + m*g*sin(slope) + brakes, translated to torque at wheel shaft
  // and added friction loss in final gear
  // in other words the torque required at prop shaft to maintain current vehicle speed

  double tq_loadWheelShaft = inputs.tq_brake + inputs.tq_env + tq_retWheel + inputs.tq_fricLoss + tq_inputWheel;

  // external load translated to prop shaft torque
  double tq_loadPropShaft = tq_loadWheelShaft / inputs.final_gear_ratio;

  // the external load is translated to a torque at the input shaft and
  // the mass of the vehicle is translated to an equivalent rotational inertia
  // at transmission input shaft

  double J_atInShaft;
  double tq_loadAtInShaft;

  if ( inputs.gear_ratio != 0 ){
    tq_loadAtInShaft = tq_loadPropShaft / inputs.gear_ratio;
    tq_loadAtInShaft += tq_inputShaft;// TODO place after if statement???

    J_atInShaft = inputs.m_vehicle * SQ ( ( inputs.r_tire / (inputs.final_gear_ratio*inputs.gear_ratio) ) );
  } else {
    // when in neutral the transmission input shaft is disconnected and the
    // speed is then integrated and the shaft inertia is set to J_neutral
    // w_inShaftNeutral is the integration result during  outside
    outputs->w_inShaft = inputs.w_inShaftNeutral;
    tq_loadAtInShaft = 0; // TODO + tq_inputShaft;???
    J_atInShaft = inputs.J_neutral;
  }

  // Clutch balance speed
  // if simplifying the engine and the vehicle as two spinning flywheels attached
  // to each plate of the clutch and then closing the clutch, the resulting
  // rotational speed of the clutch w_bal would be the weighted average
  double w_bal = (inputs.J_eng*inputs.w_eng + J_atInShaft*outputs->w_inShaft )/(inputs.J_eng+J_atInShaft);

  // calculate the torque required to accelerate the engine to w_bal in two
  // timesteps


  double tq_loadBal = (inputs.tq_eng * J_atInShaft + tq_loadAtInShaft * inputs.J_eng) / (inputs.J_eng + J_atInShaft);

  double tq_bal =  (inputs.w_eng-w_bal) * inputs.J_eng / (2*inputs.ts);
  double tq_clutchUnLim = tq_bal + tq_loadBal;

  outputs->tq_clutch = min(max(tq_clutchUnLim,-inputs.tq_clutchMax),inputs.tq_clutchMax);

  // transmission losses are given as input shaft torque loss
  double tq_inTransmission = (outputs->tq_clutch - inputs.tq_losses);

  outputs->tq_outTransmission = tq_inTransmission * inputs.gear_ratio;

  double tq_sumWheel = outputs->tq_outTransmission * inputs.final_gear_ratio - tq_loadWheelShaft;

  // w_wheel is integrate outside
  outputs->w_wheelDer = tq_sumWheel / ( inputs.m_vehicle * SQ( inputs.r_tire ) );

  outputs->v_vehicle = inputs.w_wheel * inputs.r_tire;

  // slip estimation (r_slipFilt filtered outside this m-function)
  outputs->r_slip = (inputs.tq_env + tq_sumWheel) / ( inputs.m_vehicle * 8 );

  outputs->v_driveWheel = (inputs.r_slipFilt + 1) * outputs->v_vehicle;

  outputs->w_out = outputs->v_driveWheel * inputs.final_gear_ratio / inputs.r_tire;

  if (inputs.gear_ratio == 0){
    // when gear is in neutral the input shaft speed is integrated using the
    // torque coming from the clutch
    outputs->w_inShaftDer = tq_inTransmission / inputs.J_neutral;
  }
  else{
    // When a gear is engaged the transmission input shaft speed is calculated
    // from the output shaft speed scaled with gear ratio, (the result from
    // the inputshaft neutral integration is ignored)
    outputs->w_inShaft = outputs->w_out * inputs.gear_ratio;

    // when not in neutral, set the inputShaft derivative so that the
    // integrator follows the actual speed aproximately
    outputs->w_inShaftDer = 0.5*(outputs->w_inShaft-inputs.w_inShaftNeutral)/inputs.ts;
  }

  dxdt[ 0 ]  = outputs->w_inShaftDer;
  dxdt[ 1 ]  = outputs->w_wheelDer;
  outputs->f_shaft_out = outputs->tq_clutch;
  outputs->w_shaft_out = outputs->w_inShaft;

  outputs->w_wheel_out = outputs->w_out;
  outputs->f_wheel_out = tq_sumWheel;

  return 0;
}

#define HAVE_INITIALIZATION_MODE
static int get_initial_states_size(state_t *s) {
  return 19;
}

static void get_initial_states(state_t *s, double *initials) {
  initials[0] = s->md.w_inShaftNeutral;
  initials[1] = s->md.w_wheel;
  initials[2] = s->md.w_inShaftOld;
  initials[3] = s->md.tq_retarder;
  initials[4] = s->md.tq_fricLoss;
  initials[5] = s->md.tq_env;
  initials[6] = s->md.gear_ratio;
  initials[7] = s->md.tq_clutchMax;
  initials[8] = s->md.tq_losses;
  initials[9] = s->md.r_tire;
  initials[10] = s->md.m_vehicle;
  initials[11] = s->md.final_gear_ratio;
  initials[12] = s->md.w_eng;
  initials[13] = s->md.tq_eng;
  initials[14] = s->md.J_eng;
  initials[15] = s->md.J_neutral;
  initials[16] = s->md.tq_brake;
  initials[17] = s->md.ts;
  initials[18] = s->md.r_slipFilt;
}

static int sync_out(int n, const double out[], void * params) {
  state_t *s = ( state_t * ) params;
  double * dxdt = (double * ) alloca( sizeof(double) * n );

  fcn (0, out, dxdt,  params );

  return GSL_SUCCESS;
}


static void scania_driveline_init(state_t *s) {

  double initials[19];
  get_initial_states(s, initials);

  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(get_initial_states_size(s), initials, s, fcn, NULL, NULL, NULL, 0),
      0,//s->md.filter_length,
      sync_out,
      s
    ),
    rkf45, 1e-5, 0, 0, 0, NULL
  );
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}

#ifdef CONSOLE
int main(){

  state_t s;
  s.md = defaults;
  scania_driveline_init(&s);
  s.simulation.file = fopen( "s.m", "w+" );
  s.simulation.save = 1;
  s.simulation.print = 1;
  cgsl_step_to( &s.simulation, 0.0, 40 );
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
