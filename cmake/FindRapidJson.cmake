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
#   JPEG_INCLUDE_DIR, where to find jpeglib.h, etc.
#   JPEG_LIBRARIES, the libraries needed to use JPEG.
#   JPEG_FOUND, If false, do not try to use JPEG.
#
# also defined, but not for general use are
#
# ::
#
#   JPEG_LIBRARY, where to find the JPEG library.

find_path(RapidJson_INCLUDE_DIR turbojpeg.h)


include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RapidJson DEFAULT_MSG RapidJson_INCLUDE_DIR)


# Deprecated declarations.
set (NATIVE_RapidJson_INCLUDE_PATH ${RapidJson_INCLUDE_DIR} )

mark_as_advanced(RapidJson_INCLUDE_DIR )
