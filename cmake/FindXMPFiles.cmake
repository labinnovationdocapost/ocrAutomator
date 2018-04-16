# - Try to find the XMPFiles library
#
#  XMPFiles_MIN_VERSION - You can set this variable to the minimum version you need
#                      before doing FIND_PACKAGE(XMPFiles). The default is 0.12.
#
# Once done this will define
#
#  XMPFiles_FOUND - system has libXMPFiles
#  XMPFiles_INCLUDE_DIR - the libXMPFiles include directory
#  XMPFiles_LIBRARIES - Link these to use libXMPFiles
#  XMPFiles_DEFINITIONS - Compiler switches required for using libXMPFiles
#
# The minimum required version of XMPFiles can be specified using the
# standard syntax, e.g. find_package(XMPFiles 0.17)
#
# For compatiblity, also the variable XMPFiles_MIN_VERSION can be set to the minimum version
# you need before doing FIND_PACKAGE(XMPFiles). The default is 0.12.

# Copyright (c) 2010, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Support XMPFiles_MIN_VERSION for compatibility:
if(NOT XMPFiles_FIND_VERSION)
  set(XMPFiles_FIND_VERSION "${XMPFiles_MIN_VERSION}")
endif(NOT XMPFiles_FIND_VERSION)

# the minimum version of XMPFiles we require
if(NOT XMPFiles_FIND_VERSION)
  set(XMPFiles_FIND_VERSION "0.12")
endif(NOT XMPFiles_FIND_VERSION)


if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_XMPFiles QUIET XMPFiles)
   set(XMPFiles_DEFINITIONS ${PC_XMPFiles_CFLAGS_OTHER})
endif (NOT WIN32)


find_path(XMPFiles_INCLUDE_DIR NAMES XMP.hpp
          HINTS
          ${PC_XMPFiles_INCLUDEDIR}
          ${PC_XMPFiles_INCLUDE_DIRS}
        )
			
find_library(XMPFiles_LIBRARY NAMES XMPFiles libXMPFiles
             HINTS
             ${PC_XMPFiles_LIBDIR}
             ${PC_XMPFiles_LIBRARY_DIRS}
            )


# Get the version number from XMPFiles/version.hpp and store it in the cache:
if(NOT "${XMPFiles_INCLUDE_DIR}" EQUAL "XMPFiles_INCLUDE_DIR-NOTFOUND")
    file(STRINGS ${XMPFiles_INCLUDE_DIR}/XMP_Version.h XMPFiles_MAJOR_VERSION REGEX "XMPFILES_API_VERSION_MAJOR")
    file(STRINGS ${XMPFiles_INCLUDE_DIR}/XMP_Version.h XMPFiles_MINOR_VERSION REGEX "XMPFILES_API_VERSION_MINOR")
    file(STRINGS ${XMPFiles_INCLUDE_DIR}/XMP_Version.h XMPFiles_PATH_VERSION REGEX "XMPFILES_API_VERSION_MICRO")
    string(REGEX MATCH "[0-9]+" XMPFiles_MAJOR_VERSION ${XMPFiles_MAJOR_VERSION})
    string(REGEX MATCH "[0-9]+" XMPFiles_MINOR_VERSION ${XMPFiles_MINOR_VERSION})
    string(REGEX MATCH "[0-9]+" XMPFiles_PATH_VERSION ${XMPFiles_PATH_VERSION})
    set(XMPFiles_VERSION ${XMPFiles_MAJOR_VERSION}.${XMPFiles_MINOR_VERSION}.${XMPFiles_PATH_VERSION})
endif()

set(XMPFiles_LIBRARIES "${XMPFiles_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XMPFiles  REQUIRED_VARS  XMPFiles_LIBRARY XMPFiles_INCLUDE_DIR
                                         VERSION_VAR  XMPFiles_VERSION)

mark_as_advanced(XMPFiles_INCLUDE_DIR XMPFiles_LIBRARY)

