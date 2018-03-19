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
#   MuPdf_INCLUDE_DIR, where to find jpeglib.h, etc.
#   MuPdf_LIBRARIES, the libraries needed to use JPEG.
#   MuPdf_FOUND, If false, do not try to use JPEG.
#
# also defined, but not for general use are
#
# ::
#
#   MuPdf_LIBRARY, where to find the JPEG library.

find_path(MuPdf_INCLUDE_DIR mupdf/fitz.h)

set(MuPdf_NAMES ${MuPdf_NAMES} libmupdf)
find_library(MuPdf_LIBRARY NAMES ${MuPdf_NAMES} )

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MuPdf DEFAULT_MSG MuPdf_LIBRARY MuPdf_INCLUDE_DIR)

if(MuPdf_FOUND)
  set(MuPdf_LIBRARIES ${MuPdf_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_MuPdf_INCLUDE_PATH ${MuPdf_INCLUDE_DIR} )
if(MuPdf_LIBRARY)
  get_filename_component (NATIVE_MuPdf_LIB_PATH ${MuPdf_LIBRARY} PATH)
endif()

mark_as_advanced(MuPdf_LIBRARY MuPdf_INCLUDE_DIR )
