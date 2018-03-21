# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindJPEG
# --------
#
# Find JPEG
#
# Find the native JPEG includes and library This module defines
#
# ::
#
#   JpegTurbo_INCLUDE_DIR, where to find jpeglib.h, etc.
#   JpegTurbo_LIBRARIES, the libraries needed to use JPEG.
#   JpegTurbo_FOUND, If false, do not try to use JPEG.
#
# also defined, but not for general use are
#
# ::
#
#   JpegTurbo_LIBRARY, where to find the JPEG library.

find_path(JpegTurbo_INCLUDE_DIR turbojpeg.h)

set(JpegTurbo_NAMES ${JpegTurbo_NAMES} turbojpeg)
find_library(JpegTurbo_LIBRARY NAMES ${JpegTurbo_NAMES} )

if(JpegTurbo_INCLUDE_DIR AND EXISTS "${JpegTurbo_INCLUDE_DIR}/jconfig.h")
# #define LIBJPEG_TURBO_VERSION 1.5.3
  set(_JpegTurbo_VERSION_REGEX "^#define[ \t]+LIBJPEG_TURBO_VERSION[ \t]+([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*.*$")
  file(STRINGS "${JpegTurbo_INCLUDE_DIR}/jconfig.h" _JpegTurbo_VERSION_STRING LIMIT_COUNT 1 REGEX "${_JpegTurbo_VERSION_REGEX}")
  if(_JpegTurbo_VERSION_STRING)
    string(REGEX REPLACE "${_JpegTurbo_VERSION_REGEX}" "\\1.\\2.\\3" JpegTurbo_VERSION "${_JpegTurbo_VERSION_STRING}")
  endif()
  unset(_JpegTurbo_VERSION_REGEX)
  unset(_JpegTurbo_VERSION_STRING)
endif()

if(JpegTurbo_INCLUDE_DIR AND EXISTS "${JpegTurbo_INCLUDE_DIR}/x86_64-linux-gnu/jconfig.h")
# #define LIBJPEG_TURBO_VERSION 1.5.3
  set(_JpegTurbo_VERSION_REGEX "^#define[ \t]+LIBJPEG_TURBO_VERSION[ \t]+([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*.*$")
  file(STRINGS "${JpegTurbo_INCLUDE_DIR}/x86_64-linux-gnu/jconfig.h" _JpegTurbo_VERSION_STRING LIMIT_COUNT 1 REGEX "${_JpegTurbo_VERSION_REGEX}")
  if(_JpegTurbo_VERSION_STRING)
    string(REGEX REPLACE "${_JpegTurbo_VERSION_REGEX}" "\\1.\\2.\\3" JpegTurbo_VERSION "${_JpegTurbo_VERSION_STRING}")
  endif()
  unset(_JpegTurbo_VERSION_REGEX)
  unset(_JpegTurbo_VERSION_STRING)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JpegTurbo REQUIRED_VARS JpegTurbo_LIBRARY JpegTurbo_INCLUDE_DIR VERSION_VAR JpegTurbo_VERSION)

if(JpegTurbo_FOUND)
  set(JpegTurbo_LIBRARIES ${JpegTurbo_LIBRARY})
endif()

add_library(JpegTurbo SHARED IMPORTED)
set_target_properties(JpegTurbo PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "JpegTurbo_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${JpegTurbo_INCLUDE_DIR}"
  IMPORTED_IMPLIB ${JpegTurbo_LIBRARIES}
  VERSION ${JpegTurbo_VERSION}
)

mark_as_advanced(JpegTurbo_LIBRARY JpegTurbo_INCLUDE_DIR )
