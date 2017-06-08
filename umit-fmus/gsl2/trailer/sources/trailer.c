//fix WIN32 build
#include "hypotmath.h"

#include "modelDescription.h"
#include "gsl-interface.h"

#define SIMULATION_TYPE cgsl_simulation
#define SIMULATION_EXIT_INIT trailer_init
#define SIMULATION_FREE cgsl_free_simulation

#include "fmuTemplate.h"


#if defined(_WIN32)
#include <malloc.h>
#define alloca _alloca
#else
#include <alloca.h>
#endif

const static double flip = -1;

/*
    One translational body driven by a rotational variable.

    This represents a trailer moving on a road with variable gradient,
    driven by a shaft, via a differential with gear ratio r_g, via a
    wheel of radius r_w.


    Variables are listed as:
    x    : position of trailer along road (parametric)
    v    : speed of truck
    dphi : angle difference estimate between the input shaft and the
    differential
    dx   : position difference between attached load


*/

#define SIGNUM( x ) ( (x > 0) ? 1 : ( (x < 0) ? -1 : 0 ) )

int trailer (double t, const double x[], double dxdt[], void * params){

  state_t *s = (state_t*)params;

  /* internal triangular road model, added on top of s->md.angle */
  double triangle = 0;

  if (s->md.triangle_amplitude > 0 && s->md.triangle_wavelength > 0) {
    double a = s->md.triangle_amplitude;
    double l = s->md.triangle_wavelength;
    /* modulo and divide position to 0..1, handle negative x */
    double xx = fmod((fmod(x[0], l) + l), l) / l;

    /* derivative of triangle wave is a square wave */
    triangle = -a*cos(xx*2*M_PI);
    /*if (xx < 0.5) {
      triangle = -a;
    } else {
      triangle = a;
    }*/
  }

  /* gravity */
  double force = - s->md.mass * s->md.g * sin( s->md.angle + triangle );

  double sgnv = SIGNUM( x[ 1 ] );
  /* drag */
  force += -  sgnv *   0.5 * s->md.rho  * s->md.area * s->md.c_d * x[ 1 ] * x[ 1 ];

  /* brake */
  force += - sgnv * s->md.brake * s->md.mu * s->md.g * cos( s->md.angle + triangle );

  /* rolling resistance */
  force += - sgnv * ( s->md.c_r_2 * fabs( x[ 1 ] ) + s->md.c_r_1 ) * s->md.mass * s->md.g * cos( s->md.angle + triangle );

  /* any additional force */
  force += s->md.tau_d / s->md.r_w;
  force += s->md.tau_e / s->md.r_w;

  force += s->md.f_in;

  /* coupling torque */
  s->md.tau_c =   s->md.gamma_d * ( x[ 1 ] / s->md.r_g / s->md.r_w - s->md.omega_i );

  if ( s->md.integrate_dw ) {
    s->md.tau_c +=  s->md.k_d *  x[ 2 ];
    dxdt[ 2 ] = x[ 1 ] / s->md.r_g / s->md.r_w - s->md.omega_i;
  }
  else{
    s->md.tau_c +=  s->md.k_d *  ( x[ 0 ] / s->md.r_g / s->md.r_w - s->md.phi_i );
    dxdt[ 2 ] = 0;
  }

  /* coupling force */
  s->md.f_c =   s->md.gamma_t * ( x[ 1 ] - s->md.v_in );

  if ( s->md.integrate_dx) {
    s->md.f_c +=  s->md.k_t *  x[ 3 ];
    dxdt[ 3 ] = x[ 1 ] -  s->md.v_in;
  }
  else{
    s->md.f_c +=  s->md.k_t *  ( x[ 0 ] - s->md.x_in );
    dxdt[ 3 ] = 0;
  }

  /* total acceleration */
  dxdt[ 1 ]  =  ( 1.0 / s->md.mass ) *
    ( force - s->md.f_c - s->md.tau_c / s->md.r_w );
  dxdt[ 0 ]  = x[ 1 ];

  return GSL_SUCCESS;

}



/** 
 * TODO: note that this is full of nonsmooth terms and so should proceed
 * with care, and only if needed.
 */
int jac_trailer (double t, const double x[], double *dfdx, double dfdt[], void *params)
{

  state_t *s = (state_t*)params;
  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 4, 4);
  gsl_matrix * J = &dfdx_mat.matrix;
  fprintf(stderr, "Jacobian not implemented for trailer.c\n");

  return -1;
}




#define HAVE_INITIALIZATION_MODE
static int get_initial_states_size(state_t *s) {
  return 4;
}

static void get_initial_states(state_t *s, double *initials) {
  initials[0] = s->md.x0;
  initials[1] = s->md.v0;
  initials[2] = 0;
  initials[3] = 0;
}

static int sync_out(double t, int n, const double outputs[], void * params) {
  state_t *s = ( state_t * ) params;
  double * dxdt = (double * ) alloca( sizeof(double) * n );

  trailer (0, outputs, dxdt,  params );

   s->md.x            = outputs[ 0 ];
   s->md.v            = outputs[ 1 ];
   s->md.a            = dxdt[ 1 ];

   s->md.phi   = s->md.x / s->md.r_w / s->md.r_g;
   s->md.omega = s->md.v / s->md.r_w / s->md.r_g;
   s->md.alpha = dxdt[ 1 ] / s->md.r_w / s->md.r_g;

  return GSL_SUCCESS;
}


static fmi2Status trailer_init(ModelInstance *comp) {
  state_t *s = &comp->s;

  double initials[4];
  get_initial_states(s, initials);

  s->simulation = cgsl_init_simulation(
    cgsl_epce_default_model_init(
      cgsl_model_default_alloc(get_initial_states_size(s), initials, s, trailer, NULL, NULL, NULL, 0),
      s->md.filter_length,
      sync_out,
      s
      ),
    s->md.integrator, 1e-6, 0, 0, s->md.octave_output, s->md.octave_output ? fopen(s->md.octave_output_file, "w") : NULL
    );
    return fmi2OK;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize, fmi2Boolean noSetFMUStatePriorToCurrentPoint) {
  cgsl_step_to( &s->simulation, currentCommunicationPoint, communicationStepSize );
}


#ifdef CONSOLE
int main(){
  state_t s;
  s.md = defaults;
  trailer_init(&s);
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