#include "modelDescription.h"
#include "gsl-interface.h"

typedef struct {
    cgsl_simulation sim;
    fmi2Real momega2;
    fmi2Real mzeta_cw;
} coupled_sho_simulation;

#define SIMULATION_TYPE coupled_sho_simulation
#define SIMULATION_INIT coupled_sho_init
#define SIMULATION_FREE coupled_sho_free

#include "fmuTemplate.h"

/*  The simple forced harmonic oscillator with an external coupling: 

    x'' + zeta * x'  + x = - mu * omega2 * ( dx ) - mu * omega * zeta_c * ( x' - x0' )
    dx' = x' - x0';
    
    dx is the estimated angle difference. 
    
    Also, 
    omega2 = omega * omega;

 */

static int coupled_sho (double t, const double x[], double dxdt[], void * params){

  state_t *s  = (state_t*)params;

  /** compute the coupling force: NOTE THE SIGN!
   *  This is the force *applied* to the coupled system
   */
  s->md.force_c =   s->simulation.mzeta_cw * ( x[ 1 ] - s->md.v0 )  + s->simulation.momega2 * x[ 2 ];

  dxdt[ 0 ]  = x[ 1 ];
  /** internal dynamics */ 
  dxdt[ 1 ] = - ( x[ 0 ] - s->md.x0 ) - s->md.zeta * x[ 1 ];
  /** coupling */ 
  dxdt[ 1 ] -= s->md.force_c;
  /** additional driver */ 
  dxdt[ 1 ] += s->md.force;

  dxdt[ 2 ] = x[ 1 ] - s->md.v0;

  return GSL_SUCCESS;

}



static int jac_coupled_sho (double t, const double x[], double *dfdx, double dfdt[], void *params)
{
  state_t *s  = (state_t*)params;

  gsl_matrix_view dfdx_mat = gsl_matrix_view_array (dfdx, 3, 3);
  gsl_matrix * J = &dfdx_mat.matrix; 

  /** first row */
  gsl_matrix_set (J, 0, 0, 0.0); 
  gsl_matrix_set (J, 0, 1, 1.0 ); /* position/velocity */
  gsl_matrix_set (J, 0, 2, 0.0 ); 

  /** second row */
  gsl_matrix_set (J, 1, 0, -1 );
  gsl_matrix_set (J, 1, 1, - ( s->md.zeta + s->simulation.mzeta_cw ) );
  gsl_matrix_set (J, 1, 2, - s->simulation.momega2);

  /** third row */
  gsl_matrix_set (J, 2, 0, 0.0 );
  gsl_matrix_set (J, 2, 1, 1.0 ); /* angle difference */
  gsl_matrix_set (J, 2, 2, 0.0 ); 

  dfdt[0] = 0.0;		
  dfdt[1] = 0.0;		
  dfdt[2] = 0.0; /* would have a term here if the force is
		    some polynomial interpolation */

  return GSL_SUCCESS;
}

static int epce_post_step(int n, const double outputs[], void * params) {
    state_t *s = params;

    s->md.x = outputs[0];
    s->md.v = outputs[1];
    s->md.force_c =   s->simulation.mzeta_cw * ( outputs[ 1 ] - s->md.v0 )  + s->simulation.momega2 * outputs[ 2 ];

    return GSL_SUCCESS;
}

static void coupled_sho_init(state_t *s) {
    const double initials[3] = {s->md.xstart, s->md.vstart, s->md.dxstart};

    s->simulation.momega2 = s->md.mu *s->md.omega * s->md.omega;
    s->simulation.mzeta_cw = s->md.zeta_c * s->md.omega;

    FILE *f = NULL;

    if (s->md.dump_data) {
        f = fopen("sho.m", "w");
    }

    s->simulation.sim = cgsl_init_simulation(
        cgsl_epce_default_model_init(
            cgsl_model_default_alloc(3, initials, s, coupled_sho, jac_coupled_sho, NULL, NULL, 0),
            s->md.filter_length,
            epce_post_step,
            s
        ),
        rkf45, 1e-5, 0, 0, s->md.dump_data, f
    );
}

static void coupled_sho_free(coupled_sho_simulation css) {
    cgsl_free_simulation(css.sim);
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    cgsl_step_to( &s->simulation.sim, currentCommunicationPoint, communicationStepSize );
}

//gcc -g coupled_sho.c ../../../templates/gsl2/gsl-interface.c -DCONSOLE -I../../../templates/gsl2 -I../../../templates/fmi2 -lgsl -lgslcblas -lm -Wall
#ifdef CONSOLE
int main(void) {
    state_t s = {
        {
            1.0,
            1.0,
            0.0,
            0.0,
            0.0,
            1.0, 
            0.0,
            8,
            -4,
            0,
            0,
        }
    };
    coupled_sho_init(&s);
    s.simulation.sim.file = fopen( "foo.m", "w+" );
    s.simulation.sim.save = 1;
    s.simulation.sim.print = 1;

    cgsl_step_to( &s.simulation.sim, 0.0, 10.0 );
    cgsl_free_simulation(s.simulation.sim);
    return 0;
}
#else

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"

#endif
