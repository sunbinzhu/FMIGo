/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER stringtest
#define MODEL_GUID "{2d07cf22-6bf8-440a-ba89-dff56b193db7}"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 0
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 0
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 6
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0

//will be defined in fmuTemplate.h
//needed in generated_fmi2GetX/fmi2SetX for wrapper.c
struct ModelInstance;


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {



    fmi2Char    s_out[256]; //VR=1
    fmi2Char    s_out2[256]; //VR=2
    fmi2Char    s_in[256]; //VR=3
    fmi2Char    s_in2[256]; //VR=4
    fmi2Char    s0[256]; //VR=5
    fmi2Char    s02[256]; //VR=6
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {



    "", //s_out
    "", //s_out2
    "", //s_in
    "", //s_in2
    "", //s0
    "", //s02
};





#define VR_S_OUT 1
#define VR_S_OUT2 2
#define VR_S_IN 3
#define VR_S_IN2 4
#define VR_S0 5
#define VR_S02 6

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {

        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(struct ModelInstance *comp, const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: value[i] = md->s_out; break;
        case 2: value[i] = md->s_out2; break;
        case 3: value[i] = md->s_in; break;
        case 4: value[i] = md->s_in2; break;
        case 5: value[i] = md->s0; break;
        case 6: value[i] = md->s02; break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(struct ModelInstance *comp, modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    size_t i;

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
        case 1: if (strlcpy(md->s_out, value[i], sizeof(md->s_out)) >= sizeof(md->s_out)) { return fmi2Error; } break;
        case 2: if (strlcpy(md->s_out2, value[i], sizeof(md->s_out2)) >= sizeof(md->s_out2)) { return fmi2Error; } break;
        case 3: if (strlcpy(md->s_in, value[i], sizeof(md->s_in)) >= sizeof(md->s_in)) { return fmi2Error; } break;
        case 4: if (strlcpy(md->s_in2, value[i], sizeof(md->s_in2)) >= sizeof(md->s_in2)) { return fmi2Error; } break;
        case 5: if (strlcpy(md->s0, value[i], sizeof(md->s0)) >= sizeof(md->s0)) { return fmi2Error; } break;
        case 6: if (strlcpy(md->s02, value[i], sizeof(md->s02)) >= sizeof(md->s02)) { return fmi2Error; } break;
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H