//TODO: add some kind of flag that switches this one between a clutch and a gearbox, to reduce the amount of code needed

#include "modelDescription.h"
#include "gsl-interface.h"
#include <memory.h>

typedef struct {
  cgsl_simulation sim;
  int last_gear;      /** for detecting when the gear changes */
  double delta_phi;   /** angle difference at gear change, for preventing
                       *  "springing" when changing gears */
} clutchgear_simulation;

static void clutchgear_free(clutchgear_simulation simulation) {
  cgsl_free_simulation(simulation.sim);
}

static void clutchgear_get(clutchgear_simulation *s) {
  cgsl_simulation_get(&s->sim);
}

static void clutchgear_set(clutchgear_simulation *s) {
  cgsl_simulation_set(&s->sim);
}

#define SIMULATION_TYPE clutchgear_simulation
#define SIMULATION_INIT clutch_init
#define SIMULATION_FREE clutchgear_free
#define SIMULATION_GET  clutchgear_get
#define SIMULATION_SET  clutchgear_set

#include "fmuTemplate.h"

static const double gear_ratios[] = {
   0,
   13.0,
   10.0,
   9.0,
   7.0,
   5.0,
   4.6,
   3.7,
   3.0,
   2.4,
   1.9,
   1.5,
   1.2,
   1.0,
   0.8
};

static double gear2ratio(state_t *s) {
  if (s->md.gear < 0) {
    return -gear_ratios[1];
  } else {
    int i = s->md.gear;
    int n = sizeof(gear_ratios)/sizeof(gear_ratios[0]);
    if (i >= n) i = n-1;
    return gear_ratios[i];
  }
}

/**
 *  The force function from the clutch
 */
static double fclutch( double dphi, double domega, double clutch_damping ); 
//static double fclutch_dphi_derivative( double dphi );

/*  
    Two (rotational) bodies connected via a piecewise linear clutch model. 
    
    Each body is coupled via force or velocity (or even position) with the
    outside.  

    Additional external forces are provided for each of the bodies. 

    For the case of velocity coupling, the angle difference is integrated. 

    Variables are listed as: 
    x_e    : position engine plate
    v_e    : velocity engine plate
    x_s    : position shaft plate
    v_s    : velocity shaft plate
    dx_e   : angle difference estimate between engine plate and outside coupling
    dx_s   : angle difference estimate between shaft plate and outside coupling

*/

static void compute_forces(state_t *s, const double x[], double *force_e, double *force_s, double *force_clutch) {
  int dx_s_idx = s->md.integrate_dx_e ? 5 : 4;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  *force_e =   s->md.gamma_ec * ( x[ 1 ] - s->md.v_in_e );
  if ( s->md.integrate_dx_e )
    *force_e +=  s->md.k_ec *  x[ 4 ];
  else
    *force_e +=  s->md.k_ec *  ( x[ 0 ] - s->md.x_in_e );

  *force_s =   s->md.gamma_sc * ( x[ 3 ] - s->md.v_in_s );
  if ( s->md.integrate_dx_s )
    *force_s +=  s->md.k_sc *  x[ dx_s_idx ];
  else
    *force_s +=  s->md.k_sc *  ( x[ 2 ] - s->md.x_in_s );

  if (force_clutch) {
    if (s->md.is_gearbox) {
      if (s->md.gear == 0) {
        // neutral
        *force_clutch = 0;
      } else {
        double ratio = gear2ratio(s);
        *force_clutch = s->md.gear_k * (x[ 0 ] - ratio*x[ 2 ] + s->simulation.delta_phi) + s->md.gear_d * (x[ 1 ] - ratio*x[ 3 ]);
      }
    } else {
      *force_clutch =  s->md.clutch_position * fclutch( x[ 0 ] - x[ 2 ], x[ 1 ] - x[ 3 ], s->md.clutch_damping );
    }
  }
}

int clutch (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;
  
  /** the index of dx_s depends on whether we're integrating dx_e or not */
  int dx_s_idx = s->md.integrate_dx_e ? 5 : 4;

  double force_e, force_s, force_clutch;
  compute_forces(s, x, &force_e, &force_s, &force_clutch);

  /** Second order dynamics */
  dxdt[ 0 ]  = x[ 1 ];

  /** internal dynamics */ 
  dxdt[ 1 ]  = -s->md.gamma_e * x[ 1 ];		
  /** coupling */ 
  dxdt[ 1 ] += -force_clutch;
  /** counter torque from next module */ 
  dxdt[ 1 ] += s->md.force_in_e;
  /** additional driver */ 
  dxdt[ 1 ] += s->md.force_in_ex;
  dxdt[ 1 ] -= force_e;
  dxdt[ 1 ] /= s->md.mass_e;
 
  /** shaft-side plate */

  dxdt[ 2 ]  = x[ 3 ];
  
  /** internal dynamics */ 
  dxdt[ 3 ]  = -s->md.gamma_s * x[ 3 ];
  /** coupling */ 
  dxdt[ 3 ] +=  force_clutch;
  /** counter torque from next module */ 
  dxdt[ 3 ] += s->md.force_in_s;
  /** additional driver */ 
  dxdt[ 3 ] += s->md.force_in_sx;
  dxdt[ 3 ] -= force_s;
  dxdt[ 3 ] /= s->md.mass_s;
 
 
  /** angle difference */
  if ( s->md.integrate_dx_e )
    dxdt[ 4 ]        = x[ 1 ] - s->md.v_in_e;
  if ( s->md.integrate_dx_s )
    dxdt[ dx_s_idx ] = x[ 3 ] - s->md.v_in_s;


  return GSL_SUCCESS;

}



/** TODO */
int jac_clutch (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  
  /*state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; */


  return GSL_FAILURE;
}

/**
 *  Parameters should be read from a file but that's quicker to setup.
 *  These numbers were provided by Scania.
 */
static double fclutch( double dphi, double domega, double clutch_damping ) {
  
  //Scania's clutch curve
  static const double b[] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
  static const double c[] = { -1000, -30, 0, 50, 3500 };
  size_t N = sizeof( c ) / sizeof( c[ 0 ] );
  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double tc = c[ 0 ]; //clutch torque

  if (dphi <= b[ 0 ]) {
    tc = (dphi - b[ 0 ]) / 0.034906585039886591 *  970.0 + c[ 0 ];
  } else if ( dphi >= b[ END ] ) {
    tc = ( dphi - b[ END ] ) / 0.078539816339744828 * 3450.0 + c[ END ];
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k = (dphi - b[ i ]) / (b[ i+1 ] - b[ i ]);
	tc = (1-k) * c[ i ] + k * c[ i+1 ];
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      tc = c[ END ];
    }
  }

  //add damping. 
  tc += clutch_damping * domega;

  return tc; 

}

#if 0
static double fclutch_dphi_derivative( double dphi ) {
  
  //Scania's clutch curve
  static const double b[] = { -0.087266462599716474, -0.052359877559829883, 0.0, 0.09599310885968812, 0.17453292519943295 };
  static const double c[] = { -1000, -30, 0, 50, 3500 };
  size_t N = sizeof( c ) / sizeof( c[ 0 ] );
  size_t END = N-1;
    
  /** look up internal torque based on dphi
      if too low (< -b[ 0 ]) then c[ 0 ]
      if too high (> b[ 4 ]) then c[ 4 ]
      else lerp between two values in c
  */

  double df = 0;		// clutch derivative

  if (dphi <= b[ 0 ]) {
    df =  1.0 / 0.034906585039886591 *  970.0 ;
  } else if ( dphi >= b[ END ] ) {
    df =  1.0 / 0.078539816339744828 * 3450.0 ;
  } else {
    int i;
    for (i = 0; i < END; ++i) {
      if (dphi >= b[ i ] && dphi <= b[ i+1 ]) {
	double k =  1.0  / (b[ i+1 ] - b[ i ]);
	df = k * ( c[ i + 1 ] -  c[ i ] );
	break;
      }
    }
    if (i >= END ) {
      //too high (shouldn't happen)
      df = 0;
    }
  }

  return df; 

}
#endif


static int epce_post_step(int n, const double outputs[], void * params) {

  state_t *s = params;
  double dxdt[6];

  //compute accelerations. we need them for Server::computeNumericalJacobian()
  clutch(0, outputs, dxdt, params);

  s->md.x_e = outputs[ 0 ];
  s->md.v_e = outputs[ 1 ];
  s->md.a_e = dxdt[ 1 ];
  s->md.x_s = outputs[ 2 ];
  s->md.v_s = outputs[ 3 ];
  s->md.a_s = dxdt[ 3 ];

  compute_forces(s, outputs, &s->md.force_e, &s->md.force_s, NULL);

  return GSL_SUCCESS;
}


static void clutch_init(state_t *s) {
  /** system size and layout depends on which dx's are integrated */
  int N = 4 + s->md.integrate_dx_e + s->md.integrate_dx_s;
  double initials[] = {
    s->md.x0_e,
    s->md.v0_e,
    s->md.x0_s,
    s->md.v0_s,
    s->md.dx0_e,
    s->md.dx0_s
  };

  if (!s->md.integrate_dx_e) {
    initials[4] = s->md.dx0_s;
  }

  s->simulation.sim = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(N, initials, s, clutch, jac_clutch, NULL, NULL, 0),
      s->md.filter_length,
      epce_post_step,
      s
      ),
    rkf45, 1e-5, 0, 0, s->md.octave_output, s->md.octave_output ? fopen("clutch2.m", "w") : NULL
    );
  s->simulation.last_gear = s->md.gear;
  s->simulation.delta_phi = 0;
}

#define NEW_DOSTEP //to get noSetFMUStatePriorToCurrentPoint
static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  if (s->md.is_gearbox && s->md.gear != s->simulation.last_gear) {
    /** gear changed - compute impact that keeps things sane */
    double ratio = gear2ratio(s);
    s->simulation.delta_phi = ratio*s->simulation.sim.model->x[ 2 ] - s->simulation.sim.model->x[ 0 ];
  }

  //don't dump tentative steps
  s->simulation.sim.print = noSetFMUStatePriorToCurrentPoint;
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );

  s->simulation.last_gear = s->md.gear;
}


//gcc -g clutch.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(){

  FILE * f = fopen("data.m", "w+");

  state_t s = {{
      8.0, 			/* init position */
      4.0, 			/* init velocity */
      0.0, 			/* init angle difference*/
      10.0, 			/* mass */
      2.0,			/* damping */
      10.0,			/* clutch damping */
      15.0,			/* init angular velocity */
      10,				/* input force */
    }};

  clutch_init(&s);
  s.simulation.file = fopen( "s.m", "w+" );
  s.simulation.save = 1;
  s.simulation.print = 1;
  cgsl_step_to( &s.simulation, 0.0, 10.0 );
  cgsl_free_simulation(s.simulation);

  return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
