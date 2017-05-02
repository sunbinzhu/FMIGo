#!/usr/bin/python
from __future__ import print_function
import sys, argparse, re
import xml.etree.ElementTree as e

parser = argparse.ArgumentParser(
    description='Transforms modelDescription.xml to stdout (pipe to sources/modelDescription.h)',
)

parser.add_argument('xml',
                    type=str,
                    help='Path to modelDescription.xml',
                    )
parser.add_argument('-w','--wrapper',
                    dest='wrapper',
                    action='store_true',
                    help='Generate wrapping fmi2SetX/fmi2GetX',
                    )
parser.set_defaults(wrapper=False)


def warning(*objs):
    print("WARNING: ", *objs, file=sys.stderr)

def error(*objs):
    print('#error ',*objs)
    print("ERROR: ", *objs, file=sys.stderr)

if len(sys.argv) < 2:
    print('USAGE: '+sys.argv[0] +' modelDescription-filename > header-filename', file=sys.stderr)
    print('Example: '+sys.argv[0]+' modelDescription.xml > header.h', file=sys.stderr)
    exit(1)
print('''/*This file is genereted by modeldescription2header. DO NOT EDIT! */''')

# Parse xml file
args = parser.parse_args()
tree = e.parse(args.xml)
root = tree.getroot()

# Get FMU version
fmiVersion = root.get('fmiVersion')
ni = root.get('numberOfEventIndicators');

# Get model identifier.
csFmuType = False
meFmuType = False

modelIdentifier = "modelIdentifier"
providesDirectionalDerivative = False
canGetAndSetFMUstate = False

if fmiVersion == "1.0":
    modelIdentifier = root.get('modelIdentifier')
    providesDirectionalDerivative = root.get('providesDirectionalDerivative') == "true"
elif fmiVersion == "2.0":
    me = root.find('ModelExchange')
    cs = root.find('CoSimulation')
    csFmuType = cs != None
    meFmuType = me != None

    if cs != None:
        modelIdentifier = cs.get('modelIdentifier')
        providesDirectionalDerivative = cs.get('providesDirectionalDerivative') == "true"
        canGetAndSetFMUstate = cs.get('canGetAndSetFMUstate') == "true"
    elif me != None:
        modelIdentifier = me.get('modelIdentifier')
        providesDirectionalDerivative = me.get('providesDirectionalDerivative') == "true"
        canGetAndSetFMUstate = me.get('canGetAndSetFMUstate') == "true"
    else:
        warning('FMU is neither ModelExchange or CoSimulation')
        exit(1)


reals = {}
ints  = {}
bools = {}
states = {}
derivatives = {}

SV = root.find('ModelVariables').findall('ScalarVariable')
strs  = {}

for sv in SV:
    name = sv.attrib['name']
    name = re.sub('[.\[\(]', '_', name)
    name = re.sub('[\]\)]', '', name)

    vr = int(sv.attrib['valueReference'])

    if name in reals.values() or name in ints.values() or name in bools.values():
        error(args.xml +' contains multiple variables named "' + name + '"!')
        exit(1)

    R = sv.find('Real')
    I = sv.find('Integer')
    E = sv.find('Enum')
    B = sv.find('Boolean')
    S = sv.find('String')

    if R != None:
        if vr in reals:
            error(args.xml +' contains multiple Reals with VR='+str(vr))
            exit(1)
        start = float(R.attrib['start']) if 'start' in R.attrib else 0
        if meFmuType:
            if 'derivative' in R.attrib:
                states[vr] = (SV[int(R.attrib['derivative']) - 1].attrib['name'], start)
                start = float(R.attrib['derivative']) if 'derivative' in R.attrib else 0
                derivatives[vr] = (name, start)
        reals[vr] = (name, start)

    elif I != None or E != None:
        if vr in ints:
            error(args.xml + ' contains multiple Integers/Enums with VR='+str(vr))
            exit(1)
        IE = I if I != None else E
        start = int(IE.attrib['start']) if 'start' in IE.attrib else 0
        ints[vr] = (name, start)
    elif B != None:
        if vr in bools:
            error(args.xml + ' contains multiple Booleans with VR='+str(vr))
            exit(1)
        start = B.attrib['start'] if 'start' in B.attrib else '0'
        bools[vr] = (name, 1 if start == '1' or start == 'true' else 0)
    elif S != None:
        start = S.attrib['start'] if 'start' in S.attrib else ''
        if 'size' in S.attrib:
            size = int(S.attrib['size'])
        else:
            error('String variable "%s" missing size' % name)
            exit(1)
        strs[vr] = (name, start, size)
    else:
        error('Variable "%s" has unknown/unsupported type' % name)
        exit(1)

print('''#ifndef MODELDESCRIPTION_H
#define MODELDESCRIPTION_H
#include "FMI2/fmi2Functions.h" //for fmi2Real etc.
#include "strlcpy.h" //for strlcpy()

#define MODEL_IDENTIFIER %s
#define MODEL_GUID "%s"
%s
#define HAVE_DIRECTIONAL_DERIVATIVE %i
#define CAN_GET_SET_FMU_STATE %i
#define NUMBER_OF_REALS %i
#define NUMBER_OF_INTEGERS %i
#define NUMBER_OF_BOOLEANS %i
#define NUMBER_OF_STRINGS %i
#define NUMBER_OF_STATES %i
#define NUMBER_OF_EVENT_INDICATORS %s
''' % (
    modelIdentifier,
    root.get('guid'),
    '#define FMI_COSIMULATION' if csFmuType else
    '#define FMI_MODELEXCHANGE' if meFmuType else '',
    1 if providesDirectionalDerivative else 0,
    1 if canGetAndSetFMUstate else 0,
    len(reals),
    len(ints),
    len(bools),
    len(strs),
    len(states),
    ni,
))

print('''
#define HAVE_MODELDESCRIPTION_STRUCT
typedef struct {
%s
%s
%s
%s%s
} modelDescription_t;
''' % (
    '\n'.join(['    fmi2Real    %s; //VR=%s' % (value[0], str(key)) for key,value in reals.items()]),
    '\n'.join(['    fmi2Integer %s; //VR=%s' % (value[0], str(key)) for key,value in ints.items()]),
    '\n'.join(['    fmi2Boolean %s; //VR=%s' % (value[0], str(key)) for key,value in bools.items()]),
    '\n'.join(['    fmi2Char    %s[%i]; //VR=%s' % (value[0], value[2], str(key)) for key,value in strs.items()]),
    '\n    fmi2Boolean dirty;' if meFmuType else '',
))

print('''
#define HAVE_DEFAULTS
static const modelDescription_t defaults = {
%s
%s
%s
%s%s
};
''' % (
    '\n'.join(['    %f, //%s' % (value[1], value[0]) for key,value in reals.items()]),
    '\n'.join(['    %i, //%s' % (value[1], value[0]) for key,value in ints.items()]),
    '\n'.join(['    %i, //%s' % (value[1], value[0]) for key,value in bools.items()]),
    '\n'.join(['    "%s", //%s' % (value[1], value[0]) for key,value in strs.items()]),
    '\n    1,' if meFmuType else '',
))

print('''
%s
%s
%s
%s''' % (
    '\n'.join(['#define VR_'+value[0].upper()+' '+str(key) for key,value in reals.items()]),
    '\n'.join(['#define VR_'+value[0].upper()+' '+str(key) for key,value in ints.items()]),
    '\n'.join(['#define VR_'+value[0].upper()+' '+str(key) for key,value in bools.items()]),
    '\n'.join(['#define VR_'+value[0].upper()+' '+str(key) for key,value in strs.items()]),
))

if not args.wrapper:
    print('''
//the following getters and setters are static to avoid getting linking errors if this file is included in more than one place

#define HAVE_GENERATED_GETTERS_SETTERS  //for letting the template know that we have our own getters and setters
''')

if meFmuType:
    print('''
#define STATES { %s }
#define DERIVATIVES { %s }
''' % (
    ', '.join(['VR_'+value[0].upper() for key,value in states.items()]),
    ', '.join(['VR_'+value[0].upper() for key,value in derivatives.items()]),
    ))
    print('static void update_all(modelDescription_t *md);')

def gen_getters_setters(t, d):
    print('''
static fmi2Status generated_fmi2Get%s(%smodelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, fmi2%s value[]) {
    size_t i;%s

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
%s
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}

static fmi2Status generated_fmi2Set%s(modelDescription_t *md, const fmi2ValueReference vr[], size_t nvr, const fmi2%s value[]) {
    size_t i;%s

    for (i = 0; i < nvr; i++) {
        switch (vr[i]) {
%s
        default: return fmi2Error;
        }
    }
    return fmi2OK;
}''' % (
        t,
        'const ' if csFmuType else '' ,
        t,
        '\n    if (md->dirty){\n        update_all(md);\n        md->dirty = 0;\n    }' if meFmuType else '',
        '\n'.join(['        case %i: value[i] = md->%s; break;' % (key, value[0]) for key,value in d.items()]),
        t,t,
        '\n    md->dirty = 1;\n' if meFmuType else '',
        # A bit convoluted maybe, but it works.
        # This makes sure settings strings larger than the FMU can handle results in an error, not a crash
        '\n'.join(['        case %i: if (strlcpy(md->%s, value[i], sizeof(md->%s)) >= sizeof(md->%s)) { return fmi2Error; } break;' %
                                                  (key, value[0], value[0], value[0]) for key,value in d.items()]) if t == 'String' else
        '\n'.join(['        case %i: md->%s = value[i]; break;' % (key, value[0]) for key,value in d.items()]),
    ))

if not args.wrapper:
    gen_getters_setters('Real',    reals)
    gen_getters_setters('Integer', ints)
    gen_getters_setters('Boolean', bools)
    gen_getters_setters('String',  strs)

print('#endif //MODELDESCRIPTION_H')
