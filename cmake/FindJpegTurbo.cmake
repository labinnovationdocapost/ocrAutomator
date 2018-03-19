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

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JpegTurbo DEFAULT_MSG JpegTurbo_LIBRARY JpegTurbo_INCLUDE_DIR)

if(JpegTurbo_FOUND)
  set(JpegTurbo_LIBRARIES ${JpegTurbo_LIBRARY})
endif()

# Deprecated declarations.
set (NATIVE_JpegTurbo_INCLUDE_PATH ${JpegTurbo_INCLUDE_DIR} )
if(JpegTurbo_LIBRARY)
  get_filename_component (NATIVE_JpegTurbo_LIB_PATH ${JpegTurbo_LIBRARY} PATH)
endif()

mark_as_advanced(JpegTurbo_LIBRARY JpegTurbo_INCLUDE_DIR )
