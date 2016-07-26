#include "modelDescription.h"

typedef struct {
    int current_step;
} impulse_simulation;

#define SIMULATION_TYPE impulse_simulation
#define SIMULATION_INIT impulse_init

#include "fmuTemplate.h"

#define PULSE_TYPE_THETA 0
#define PULSE_TYPE_OMEGA 1

static void pulse_for_current_step(state_t *s) {
    if (s->md.pulse_type == PULSE_TYPE_THETA) {
        if (s->simulation.current_step >= s->md.pulse_start &&
            s->simulation.current_step <  s->md.pulse_start + s->md.pulse_length) {
            s->md.theta = s->md.dc_offset + s->md.pulse_amplitude;
        } else {
            s->md.theta = s->md.dc_offset;
        }
        s->md.omega = 0;
    } else {    //PULSE_TYPE_OMEGA
        if (s->simulation.current_step >= s->md.pulse_start &&
            s->simulation.current_step <  s->md.pulse_start + s->md.pulse_length) {
            s->md.omega = s->md.dc_offset + s->md.pulse_amplitude;
        } else {
            s->md.omega = s->md.dc_offset;
        }
    }
}

static void impulse_init(state_t *s) {
    pulse_for_current_step(s);
}

//returns partial derivative of vr with respect to wrt
static fmi2Status getPartial(state_t *s, fmi2ValueReference vr, fmi2ValueReference wrt, fmi2Real *partial) {
    if (vr == VR_ALPHA && wrt == VR_TAU) {
        *partial = 0;
        return fmi2OK;
    }
    return fmi2Error;
}

static void doStep(state_t *s, fmi2Real currentCommunicationPoint, fmi2Real communicationStepSize) {
    if (s->md.pulse_type == PULSE_TYPE_OMEGA) {
        s->md.theta += s->md.omega * communicationStepSize;
    }

    s->simulation.current_step++;
    pulse_for_current_step(s);
}

// include code that implements the FMI based on the above definitions
#include "fmuTemplate_impl.h"
