if (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_RTTR QUIET RTTR)
   set(RTTR_DEFINITIONS ${PC_RTTR_CFLAGS_OTHER})
endif (NOT WIN32)


find_path(RTTR_INCLUDE_DIR NAMES rttr/registration.h
          HINTS
          ${PC_RTTR_INCLUDEDIR}
          ${PC_RTTR_INCLUDE_DIRS}
        )

		
# Create imported target RTTR::Core
add_library(RTTR::Core SHARED IMPORTED)


find_library(RTTR_LIBRARY NAMES RTTR libRTTR rttr_core
             HINTS
             ${PC_RTTR_LIBDIR}
             ${PC_RTTR_LIBRARY_DIRS}
            )


# Get the version number from RTTR/version.hpp and store it in the cache:
if(NOT "${RTTR_INCLUDE_DIR}" EQUAL "RTTR_INCLUDE_DIR-NOTFOUND")
    file(STRINGS ${RTTR_INCLUDE_DIR}/rttr/detail/base/version.h RTTR_MAJOR_VERSION REGEX "RTTR_VERSION_MAJOR")
    file(STRINGS ${RTTR_INCLUDE_DIR}/rttr/detail/base/version.h RTTR_MINOR_VERSION REGEX "RTTR_VERSION_MINOR")
    file(STRINGS ${RTTR_INCLUDE_DIR}/rttr/detail/base/version.h RTTR_PATH_VERSION REGEX "RTTR_VERSION_PATCH")
    string(REGEX MATCH "[0-9]+" RTTR_MAJOR_VERSION ${RTTR_MAJOR_VERSION})
    string(REGEX MATCH "[0-9]+" RTTR_MINOR_VERSION ${RTTR_MINOR_VERSION})
    string(REGEX MATCH "[0-9]+" RTTR_PATH_VERSION ${RTTR_PATH_VERSION})
    set(RTTR_VERSION ${RTTR_MAJOR_VERSION}.${RTTR_MINOR_VERSION}.${RTTR_PATH_VERSION})
endif()

set(RTTR_LIBRARIES "${RTTR_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RTTR  REQUIRED_VARS  RTTR_LIBRARY RTTR_INCLUDE_DIR
                                         VERSION_VAR  RTTR_VERSION)

set_target_properties(RTTR::Core PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "RTTR_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${RTTR_INCLUDE_DIR}"
  IMPORTED_IMPLIB ${RTTR_LIBRARY}
)

mark_as_advanced(RTTR_INCLUDE_DIR RTTR_LIBRARY)

