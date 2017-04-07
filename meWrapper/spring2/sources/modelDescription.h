/*This file is genereted by modeldescription2header. DO NOT EDIT! */
#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER wrapper_springs2
#define MODEL_GUID "10f53b77-1c5a-4ca9-a7bf-9af818ace9a6"
#define FMI_COSIMULATION
#define HAVE_DIRECTIONAL_DERIVATIVE 1
#define CAN_GET_SET_FMU_STATE 1
#define NUMBER_OF_REALS 28
#define NUMBER_OF_INTEGERS 0
#define NUMBER_OF_BOOLEANS 0
#define NUMBER_OF_STRINGS 2
#define NUMBER_OF_STATES 0
#define NUMBER_OF_EVENT_INDICATORS 0


#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
    fmi2Real    x1; //VR=1
    fmi2Real    x2; //VR=3
    fmi2Real    v1; //VR=5
    fmi2Real    a1; //VR=7
    fmi2Real    v2; //VR=8
    fmi2Real    a2; //VR=10
    fmi2Real    k1; //VR=11
    fmi2Real    gamma1; //VR=12
    fmi2Real    omega1; //VR=13
    fmi2Real    phi1; //VR=14
    fmi2Real    fc1; //VR=15
    fmi2Real    k2; //VR=16
    fmi2Real    gamma2; //VR=17
    fmi2Real    omega2; //VR=18
    fmi2Real    phi2; //VR=19
    fmi2Real    fc2; //VR=20
    fmi2Real    k_internal; //VR=21
    fmi2Real    gamma_internal; //VR=22
    fmi2Real    f1; //VR=23
    fmi2Real    f2; //VR=24
    fmi2Real    x1_i; //VR=25
    fmi2Real    v1_i; //VR=26
    fmi2Real    x2_i; //VR=27
    fmi2Real    v2_i; //VR=28
    fmi2Real    m1; //VR=29
    fmi2Real    m1i_o; //VR=30
    fmi2Real    m2; //VR=31
    fmi2Real    m2i_o; //VR=32


    fmi2Char    fmu[17]; //VR=0
    fmi2Char    directional[20]; //VR=1
} modelDescription_t;


#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
    0.000000, //x1
    0.000000, //x2
    0.000000, //v1
    0.000000, //a1
    0.000000, //v2
    0.000000, //a2
    0.000000, //k1
    0.000000, //gamma1
    0.000000, //omega1
    0.000000, //phi1
    0.000000, //fc1
    0.000000, //k2
    0.000000, //gamma2
    0.000000, //omega2
    0.000000, //phi2
    0.000000, //fc2
    0.000000, //k_internal
    0.000000, //gamma_internal
    0.000000, //f1
    0.000000, //f2
    0.000000, //x1_i
    0.000000, //v1_i
    0.000000, //x2_i
    0.000000, //v2_i
    1.000000, //m1
    0.000000, //m1i_o
    1.000000, //m2
    0.000000, //m2i_o


    "fmu/springs2.fmu", //fmu
    "", //directional
};


#define VR_X1 1
#define VR_X2 3
#define VR_V1 5
#define VR_A1 7
#define VR_V2 8
#define VR_A2 10
#define VR_K1 11
#define VR_GAMMA1 12
#define VR_OMEGA1 13
#define VR_PHI1 14
#define VR_FC1 15
#define VR_K2 16
#define VR_GAMMA2 17
#define VR_OMEGA2 18
#define VR_PHI2 19
#define VR_FC2 20
#define VR_K_INTERNAL 21
#define VR_GAMMA_INTERNAL 22
#define VR_F1 23
#define VR_F2 24
#define VR_X1_I 25
#define VR_V1_I 26
#define VR_X2_I 27
#define VR_V2_I 28
#define VR_M1 29
#define VR_M1I_O 30
#define VR_M2 31
#define VR_M2I_O 32


#define VR_FMU 0
#define VR_DIRECTIONAL 1

//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters


static fmi2Status generated_fmi2GetReal(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    fmi2_import_get_real(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetReal(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_real(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetInteger(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {
    fmi2_import_get_integer(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetInteger(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_integer(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetBoolean(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {
    fmi2_import_get_boolean(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetBoolean(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_boolean(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2GetString(const modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2String value[]) {
    fmi2_import_get_string(*getFMU(),vr,nvr,value);
    return fmi2OK;
}

static fmi2Status generated_fmi2SetString(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2String value[]) {
    if( *getFMU() != NULL)
        fmi2_import_set_string(*getFMU(),vr,nvr,value);
    return fmi2OK;
}
#endif //MODELDESCRIPTION_H
