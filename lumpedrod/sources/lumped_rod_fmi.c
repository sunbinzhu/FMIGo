#include "lumped_rod.h"
#include <math.h>
#include "modelDescription.h"


#define SIMULATION_TYPE lumped_rod_sim
//called after getting default values from XML
#define SIMULATION_INIT setStartValues
#define SIMULATION_FREE lumped_rod_sim_free_a
#define SIMULATION_GET lumped_rod_sim_store
#define SIMULATION_SET lumped_rod_sim_restore

#include "fmuTemplate.h"

static char filename [] = "lumpdata.dat";
static FILE * data_file;

static void lumped_rod_sim_free_a( lumped_rod_sim  sim    ){

  lumped_rod_sim_free( sim ) ;
  fclose( data_file);
  
}
static void lumped_rod_fmi_sync_out( lumped_rod_sim * sim, state_t *s){
  
  s->md.theta1  = sim->state.state.x1;
  s->md.theta2  = sim->state.state.xN;
  s->md.omega1  = sim->state.state.v1;
  s->md.omega2  = sim->state.state.vN;
  s->md.alpha1  = sim->state.state.a1;
  s->md.alpha2  = sim->state.state.aN;

  s->md.dtheta1  = sim->state.state.dx1;
  s->md.dtheta2  = sim->state.state.dxN;

  s->md.out_torque1  = sim->state.state.f1;
  s->md.out_torque2  = sim->state.state.fN;

  return;
  
}

static void lumped_rod_fmi_sync_in( lumped_rod_sim * sim, state_t *s){
  
  sim->state.state.driver_f1         =  s->md.tau1;
  sim->state.state.driver_fN         =  s->md.tau2;
  sim->state.state.driver_v1         =  s->md.omega_drive1;
  sim->state.state.driver_vN         =  s->md.omega_drive2;

  return;

}
 
/**
   Instantiate the simulation and set initial conditions.
*/
static void setStartValues(state_t *s) {
  /** read the init values given by the master, either from command line
      arguments or as defaults from modelDescription.xml
  */
  lumped_rod_sim_parameters p = { 
    s->md.step,
    {
      s->md.theta01, //VR=0
      s->md.theta02, //VR=1
      s->md.omega01, //VR=2
      s->md.omega02, //VR=3
      s->md.alpha1, //VR=4
      s->md.alpha2, //VR=5
      s->md.dtheta1, //VR=6
      s->md.dtheta2, //VR=7
      s->md.out_torque1, //VR=8
      s->md.out_torque2, //VR=9
      s->md.tau1, //VR=10
      s->md.tau2, //VR=11
      s->md.omega_drive1, //VR=12
      s->md.omega_drive2 //VR=13
    }, 
    {
      s->md.n_elements, //VR=0
      s->md.J0, //VR=14
      s->md.K, //VR=15
      s->md.D, //VR=16
      s->md.K_drive1, //VR=17
      s->md.D_drive1, //VR=18
      s->md.K_drive2, //VR=19
      s->md.D_drive2,  //VR=20
      s->md.driver_sign1, // VR=26
      s->md.driver_sign2 // VR=27
    }
  };
  s->simulation = lumped_rod_sim_create( p ); 

  data_file  = fopen(filename, "w+");
  for ( int i = 0; i < s->md.n_elements; ++i ){
    fprintf(data_file, " %f ", s->simulation.rod.state.x[ i ] );
  }
  fprintf(data_file, "\n");
    

};

/** Returns partial derivative of vr with respect to wrt  
 *  We could define a smart convention here.  
 */ 
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
  if (vr == VR_ALPHA1 && wrt == VR_TAU1 ) {
    *partial = s->simulation.rod.mobility[ 0 ];
    return fmi2OK;
  }

  if (vr == VR_ALPHA1 && wrt == VR_TAU2 ) {
    *partial = s->simulation.rod.mobility[ 1 ];
    return fmi2OK;
  }

  if (vr == VR_ALPHA2 && wrt == VR_TAU1 ) {
    *partial = s->simulation.rod.mobility[ 2 ];
    return fmi2OK;
  }
    
  if (vr == VR_ALPHA2 && wrt == VR_TAU2 ) {
    *partial = s->simulation.rod.mobility[ 3 ];
    return fmi2OK;
  }

  return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  /*  Copy the input variable from the state vector */
  lumped_rod_fmi_sync_in(&s->simulation, s);

  int n = ( int ) ceil( communicationStepSize / s->simulation.state.step );
  /* Execute the simulation */
  rod_sim_do_step(&s->simulation , n );
  /* Copy state variables to ouputs */
  lumped_rod_fmi_sync_out(&s->simulation, s);
  for ( int i = 0; i < s->md.n_elements; ++i ){
    fprintf(data_file, " %f ", s->simulation.rod.state.x[ i ] );
  }
  fprintf(data_file, "\n");
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

