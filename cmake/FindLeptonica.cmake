#
# Find Leptonica
#
# Exported variables:
#    Leptonica_FOUND
#    Leptonica_INCLUDE_DIRS
#    Leptonica_LIBRARIES
#
#    Leptonica_VERSION
#    Leptonica_MAJOR_VERSION
#    Leptonica_MINOR_VERSION
#

find_path(Leptonica_INCLUDE_DIR leptonica/allheaders.h
    HINTS
    /usr/include
    /usr/local/include
    /opt/include
    /opt/local/include
    ${Leptonica_DIR}/include
)
#define LIBLEPT_PATCH_VERSION   4
if(NOT "${Leptonica_INCLUDE_DIR}" EQUAL "Leptonica_INCLUDE_DIR-NOTFOUND")
    set(Leptonica_INCLUDE_DIRS ${Leptonica_INCLUDE_DIR}/leptonica)
    file(STRINGS ${Leptonica_INCLUDE_DIRS}/allheaders.h Leptonica_MAJOR_VERSION REGEX "LIBLEPT_MAJOR_VERSION")
    file(STRINGS ${Leptonica_INCLUDE_DIRS}/allheaders.h Leptonica_MINOR_VERSION REGEX "LIBLEPT_MINOR_VERSION")
    file(STRINGS ${Leptonica_INCLUDE_DIRS}/allheaders.h Leptonica_PATH_VERSION REGEX "LIBLEPT_PATCH_VERSION")
    string(REGEX MATCH "[0-9]+" Leptonica_MAJOR_VERSION ${Leptonica_MAJOR_VERSION})
    string(REGEX MATCH "[0-9]+" Leptonica_MINOR_VERSION ${Leptonica_MINOR_VERSION})
    string(REGEX MATCH "[0-9]+" Leptonica_PATH_VERSION ${Leptonica_PATH_VERSION})
    set(Leptonica_VERSION ${Leptonica_MAJOR_VERSION}.${Leptonica_MINOR_VERSION}.${Leptonica_PATH_VERSION})
endif()

find_library(Leptonica_LIBRARY NAMES lept liblept leptonica-${Leptonica_VERSION}
    HINTS
    /usr/lib
    /usr/local/lib
    /opt/lib
    /opt/local/lib
    ${Leptonica_DIR}/lib
)
set(Leptonica_LIBRARIES ${Leptonica_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Leptonica
    REQUIRED_VARS
        Leptonica_INCLUDE_DIRS
        Leptonica_LIBRARIES
    VERSION_VAR Leptonica_VERSION
    FAIL_MESSAGE "Try to set Leptonica_DIR or Leptonica_ROOT"
)

mark_as_advanced(Leptonica_INCLUDE_DIRS Leptonica_LIBRARIES)

