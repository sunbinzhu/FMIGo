#    Copyright (C) 2012 Modelon AB

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the BSD style license.

# #    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    FMILIB_License.txt file for more details.

#    You should have received a copy of the FMILIB_License.txt file
#    along with this program. If not, contact Modelon AB <http://www.modelon.com>.

if(NOT FMIXMLDIR)
set(FMIXMLDIR ${FMILIBRARYHOME}/src/XML/)
include(jmutil)

# set(DOXYFILE_EXTRA_SOURCES "${DOXYFILE_EXTRA_SOURCES} \"${FMIXMLDIR}/include\"")

include_directories("${FMIXMLDIR}/include" "${FMILIB_THIRDPARTYLIBS}/FMI/")
set(FMIXML_LIBRARIES fmixml)
set(FMIXML_EXPAT_DIR "${FMILIB_THIRDPARTYLIBS}/Expat/expat-2.1.0") 

set(FMIXMLHEADERS
	include/FMI/fmi_xml_context.h
	src/FMI/fmi_xml_context_impl.h

    include/FMI1/fmi1_xml_model_description.h
    src/FMI1/fmi1_xml_model_description_impl.h
    src/FMI1/fmi1_xml_parser.h
    include/FMI1/fmi1_xml_type.h
    src/FMI1/fmi1_xml_type_impl.h
    include/FMI1/fmi1_xml_unit.h
    src/FMI1/fmi1_xml_unit_impl.h
    include/FMI1/fmi1_xml_vendor_annotations.h
    src/FMI1/fmi1_xml_vendor_annotations_impl.h
    include/FMI1/fmi1_xml_variable.h
    src/FMI1/fmi1_xml_variable_impl.h
    include/FMI1/fmi1_xml_capabilities.h
    src/FMI1/fmi1_xml_capabilities_impl.h

    include/FMI2/fmi2_xml_model_description.h
    src/FMI2/fmi2_xml_model_description_impl.h
    include/FMI2/fmi2_xml_model_structure.h
    src/FMI2/fmi2_xml_model_structure_impl.h
    src/FMI2/fmi2_xml_parser.h
    include/FMI2/fmi2_xml_type.h
    src/FMI2/fmi2_xml_type_impl.h
    include/FMI2/fmi2_xml_unit.h
    src/FMI2/fmi2_xml_unit_impl.h
    include/FMI2/fmi2_xml_variable.h
    src/FMI2/fmi2_xml_variable_impl.h
 )

set(FMIXMLSOURCE
	src/FMI/fmi_xml_context.c

    src/FMI1/fmi1_xml_parser.c
    src/FMI1/fmi1_xml_model_description.c
    src/FMI1/fmi1_xml_type.c
    src/FMI1/fmi1_xml_unit.c
    src/FMI1/fmi1_xml_vendor_annotations.c
    src/FMI1/fmi1_xml_variable.c
    src/FMI1/fmi1_xml_capabilities.c
    src/FMI1/fmi1_xml_cosim.c

    src/FMI2/fmi2_xml_parser.c
    src/FMI2/fmi2_xml_model_description.c
    src/FMI2/fmi2_xml_model_structure.c
    src/FMI2/fmi2_xml_type.c
    src/FMI2/fmi2_xml_unit.c
	src/FMI2/fmi2_xml_vendor_annotations.c
	src/FMI2/fmi2_xml_variable.c
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DXML_STATIC -DFMI_XML_QUERY")

### Add the options of expat to the cache and add it as subdirectory.
set(BUILD_tools OFF CACHE BOOL "build the xmlwf tool for expat library")
set(BUILD_examples OFF CACHE BOOL "build the examples for expat library")
set(BUILD_tests OFF CACHE BOOL "build the tests for expat library")
set(BUILD_shared OFF CACHE BOOL "build a shared expat library")
add_subdirectory(ThirdParty/Expat/expat-2.1.0)

set(EXPAT_INCLUDE_DIRS ThirdParty/Expat/expat-2.1.0/lib)

include_directories("${EXPAT_INCLUDE_DIRS}" "${FMILIB_THIRDPARTYLIBS}/FMI/")

PREFIXLIST(FMIXMLSOURCE  ${FMIXMLDIR}/)
PREFIXLIST(FMIXMLHEADERS ${FMIXMLDIR}/)

debug_message(STATUS "adding fmixml")

add_library(fmixml ${FMILIBKIND} ${FMIXMLSOURCE} ${FMIXMLHEADERS})

target_link_libraries(fmixml ${JMUTIL_LIBRARIES} expat)

endif(NOT FMIXMLDIR)
