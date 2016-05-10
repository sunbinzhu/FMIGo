#include "lumped_rod.h"
#include <math.h>



#define MODEL_IDENTIFIER lumpedrod
#define MODEL_GUID "{b8998512-96a7-4e6d-8350-6d1f9aeae4a1}"

enum {
  THETA1,      //angle (output, state)
  THETA2,      //angle (output, state)
  OMEGA1,      //angular velocity (output, state)
  OMEGA2,      //angular velocity (output, state)
  ALPHA1,      //angular acceleration (output)
  ALPHA2,      //angular acceleration (output)
  TAU1,        //coupling torque (input)
  TAU2,        //coupling torque (input)
  J0,          // moment of inertia [1/(kg*m^2)] (parameter)
  COMP,           // compliance of the rod
  DAMP,          //drag (parameter)
  STEP,          // *internal* time step
    NUMBER_OF_REALS
};

enum {
  NELEM,                            // number of elements
  NUMBER_OF_INTEGERS
};


#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0
#define FMI_COSIMULATION

#define SIMULATION_TYPE lumped_rod_sim
#define SIMULATION_INIT setStartValues  //called after getting default values from XML
#define SIMULATION_FREE lumped_rod_sim_delete
#define SIMULATION_GET lumped_rod_sim_store
#define SIMULATION_SET lumped_rod_sim_restore

#include "fmuTemplate.h"


static void lumped_rod_fmi_sync_out( lumped_rod_sim * sim, state_t *s){
  
  s->r[ THETA1 ]  = sim->state.x1;
  s->r[ THETA2 ]  = sim->state.xN;
  s->r[ OMEGA1 ]  = sim->state.v1;
  s->r[ OMEGA2 ]  = sim->state.vN;
  s->r[ ALPHA1 ]  = sim->state.a1;
  s->r[ ALPHA2 ]  = sim->state.aN;
  s->r[ TAU1 ]    = sim->state.f1;
  s->r[ TAU2 ]    = sim->state.fN;

  return;
  
}

static void lumped_rod_fmi_sync_in( lumped_rod_sim * sim, state_t *s){
  
  sim->forces[ 0 ] =  s->r[ TAU1 ];
  sim->forces[ 1 ] =  s->r[ TAU2 ];

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
    s->r[ J0 ], 
    s->i[ NELEM ],
    s->r[ COMP ],
    s->r[ STEP ],
    s->r[ DAMP ],
    s->r[ THETA1 ],
    s->r[ THETA2 ],
    s->r[ OMEGA1 ],
    s->r[ OMEGA2 ],
    s->r[ TAU1 ],
    s->r[ TAU2 ]
  };

 
  /** WARNING: hack!  Default values didn't work as time of writing */
  p.N = 100; p.mass = 1000.0; p.compliance = 1e-3; p.step  = 0.01;
  p.tau  = 0.1; p.x1 = 0; p.xN = 0; p.v1 = 0; 
  p.vN = 0; p.f1 =  1e3; p.fN = -1e3; 

  s->simulation = lumped_rod_sim_create( p ); 

}

/** Returns partial derivative of vr with respect to wrt  
 *  We could define a smart convention here.  
*/ 
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == ALPHA1 && wrt == TAU1 ) {
      *partial = s->simulation.rod.mobility[ 0 ];
        return fmi2OK;
    }

    if (vr == ALPHA1 && wrt == TAU2 ) {
      *partial = s->simulation.rod.mobility[ 1 ];
        return fmi2OK;
    }

    if (vr == ALPHA2 && wrt == TAU1 ) {
      *partial = s->simulation.rod.mobility[ 2 ];
        return fmi2OK;
    }
    
    if (vr == ALPHA2 && wrt == TAU2 ) {
      *partial = s->simulation.rod.mobility[ 3 ];
        return fmi2OK;
    }

    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
  /*  Copy the input variable from the state vector */
  lumped_rod_fmi_sync_in(&s->simulation, s);

  int n = ( int ) ceil( communicationStepSize / s->simulation.step );
  /* Execute the simulation */
  step_rod_sim(&s->simulation , n );
  /* Copy state variables to ouputs */
  lumped_rod_fmi_sync_out(&s->simulation, s);
  
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

