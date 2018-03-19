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

find_path(A2ia_INCLUDE_DIR A2iATextReader.h $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)
find_path(A2ia_LIBRARY A2iATextReader.lib $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)
find_path(A2ia_DLL A2iATextReader.dll $ENV{PROGRAMFILES}/A2iA/A2iA\ TextReader\ V5.0/Interface)

add_library(A2ialib SHARED IMPORTED)
set_target_properties(A2ialib PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "A2ia_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${A2ia_INCLUDE_DIR}"
  
  IMPORTED_IMPLIB "${A2ia_LIBRARY}/A2iATextReader.lib"
  IMPORTED_LOCATION "${A2ia_LIBRARY}/A2iATextReader.dll"
)
