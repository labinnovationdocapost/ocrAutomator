# - Try to find the XMPCore library
#
#  XMPCore_MIN_VERSION - You can set this variable to the minimum version you need
#                      before doing FIND_PACKAGE(XMPCore). The default is 0.12.
#
# Once done this will define
#
#  XMPCore_FOUND - system has libXMPCore
#  XMPCore_INCLUDE_DIR - the libXMPCore include directory
#  XMPCore_LIBRARIES - Link these to use libXMPCore
#  XMPCore_DEFINITIONS - Compiler switches required for using libXMPCore
#
# The minimum required version of XMPCore can be specified using the
# standard syntax, e.g. find_package(XMPCore 0.17)
#
# For compatiblity, also the variable XMPCore_MIN_VERSION can be set to the minimum version
# you need before doing FIND_PACKAGE(XMPCore). The default is 0.12.

# Copyright (c) 2010, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2008, Gilles Caulier, <caulier.gilles@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Support XMPCore_MIN_VERSION for compatibility:
if(NOT XMPCore_FIND_VERSION)
  set(XMPCore_FIND_VERSION "${XMPCore_MIN_VERSION}")
endif(NOT XMPCore_FIND_VERSION)

# the minimum version of XMPCore we require
if(NOT XMPCore_FIND_VERSION)
  set(XMPCore_FIND_VERSION "0.12")
endif(NOT XMPCore_FIND_VERSION)


if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_XMPCore QUIET XMPCore)
   set(XMPCore_DEFINITIONS ${PC_XMPCore_CFLAGS_OTHER})
endif (NOT WIN32)


find_path(XMPCore_INCLUDE_DIR NAMES XMP.hpp
          HINTS
          ${PC_XMPCore_INCLUDEDIR}
          ${PC_XMPCore_INCLUDE_DIRS}
        )

find_library(XMPCore_LIBRARY NAMES XMPCore libXMPCore
             HINTS
             ${PC_XMPCore_LIBDIR}
             ${PC_XMPCore_LIBRARY_DIRS}
            )
			

# Get the version number from XMPCore/version.hpp and store it in the cache:
if(NOT "${XMPCore_INCLUDE_DIR}" EQUAL "XMPCore_INCLUDE_DIR-NOTFOUND")
    file(STRINGS ${XMPCore_INCLUDE_DIR}/XMP_Version.h XMPCore_MAJOR_VERSION REGEX "XMPCORE_API_VERSION_MAJOR")
    file(STRINGS ${XMPCore_INCLUDE_DIR}/XMP_Version.h XMPCore_MINOR_VERSION REGEX "XMPCORE_API_VERSION_MINOR")
    file(STRINGS ${XMPCore_INCLUDE_DIR}/XMP_Version.h XMPCore_PATH_VERSION REGEX "XMPCORE_API_VERSION_MICRO")
    string(REGEX MATCH "[0-9]+" XMPCore_MAJOR_VERSION ${XMPCore_MAJOR_VERSION})
    string(REGEX MATCH "[0-9]+" XMPCore_MINOR_VERSION ${XMPCore_MINOR_VERSION})
    string(REGEX MATCH "[0-9]+" XMPCore_PATH_VERSION ${XMPCore_PATH_VERSION})
    set(XMPCore_VERSION ${XMPCore_MAJOR_VERSION}.${XMPCore_MINOR_VERSION}.${XMPCore_PATH_VERSION})
endif()

set(XMPCore_LIBRARIES "${XMPCore_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XMPCore  REQUIRED_VARS  XMPCore_LIBRARY XMPCore_INCLUDE_DIR
                                         VERSION_VAR  XMPCore_VERSION)

mark_as_advanced(XMPCore_INCLUDE_DIR XMPCore_LIBRARY)

