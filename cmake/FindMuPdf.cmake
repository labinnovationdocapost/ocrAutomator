# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindMuPdf
# --------
#
# Find MuPdf
#
# Find the native JPEG includes and library This module defines
#
# ::
#
#   MuPdf_INCLUDE_DIR, where to find fitz.h, etc.
#   MuPdf_LIBRARIES, the libraries needed to use MuPdf.
#   MuPdf_FOUND, If false, do not try to use MuPdf.
#
# also defined, but not for general use are
#
# ::
#
#   MuPdf_LIBRARY, where to find the MuPdf library.

find_path(MuPdf_INCLUDE_DIR mupdf/fitz.h)

find_library(MuPdf_LIBRARY NAMES mupdf libmupdf )
find_library(MuPdf_LIBRARY_TIER NAMES mupdfthird libmupdfthird )



if(MuPdf_INCLUDE_DIR AND EXISTS "${MuPdf_INCLUDE_DIR}/mupdf/fitz/version.h")
# #define FZ_VERSION "1.12.0"
  set(_MuPdf_VERSION_REGEX "^#define[ \t]+FZ[_A-Z]+VERSION[_A-Z]*[ \t]+\"([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*\".*$")
  file(STRINGS "${MuPdf_INCLUDE_DIR}/mupdf/fitz/version.h" _MuPdf_VERSION_STRING LIMIT_COUNT 1 REGEX "${_MuPdf_VERSION_REGEX}")
  if(_MuPdf_VERSION_STRING)
    string(REGEX REPLACE "${_MuPdf_VERSION_REGEX}" "\\1.\\2.\\3" MuPdf_VERSION "${_MuPdf_VERSION_STRING}")
  endif()
  unset(_MuPdf_VERSION_REGEX)
  unset(_MuPdf_VERSION_STRING)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MuPdf REQUIRED_VARS MuPdf_LIBRARY MuPdf_INCLUDE_DIR VERSION_VAR MuPdf_VERSION)

if(MuPdf_FOUND)
  set(MuPdf_LIBRARIES ${MuPdf_LIBRARY})
endif()
if(MuPdf_LIBRARY_TIER)
  list(APPEND MuPdf_LIBRARIES ${MuPdf_LIBRARY_TIER})
endif()


add_library(MuPdfLib SHARED IMPORTED)
set_target_properties(MuPdfLib PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "MuPdf_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${MuPdf_INCLUDE_DIR}"
  IMPORTED_IMPLIB ${MuPdf_LIBRARY}
  VERSION ${MuPdf_VERSION}
)

mark_as_advanced(MuPdf_LIBRARY MuPdf_INCLUDE_DIR )
