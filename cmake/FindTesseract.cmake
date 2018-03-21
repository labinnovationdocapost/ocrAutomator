# - Try to find Tesseract-OCR
# Once done, this will define
#
#  Tesseract_FOUND - system has Tesseract
#  Tesseract_INCLUDE_DIRS - the Tesseract include directories
#  Tesseract_LIBRARIES - link these to use Tesseract

find_path(Tesseract_INCLUDE_DIR tesseract/baseapi.h
    HINTS
    /usr/include
    /usr/local/include
    /opt/include
    /opt/local/include
    "c:/program files/tesseract/include"
    ${Tesseract_DIR}/include
)

if(Tesseract_INCLUDE_DIR AND EXISTS "${Tesseract_INCLUDE_DIR}/tesseract/version.h")
# #define LIBJPEG_TURBO_VERSION 1.5.3
    set(Tesseract_INCLUDE_DIRS ${Tesseract_INCLUDE_DIR}/tesseract/)
  set(_Tesseract_VERSION_REGEX "^#define[ \t]+TESSERACT_VERSION_STR[ \t]+\"([0-9]+)\\.([0-9]+)\\.([0-9]+)[^\"]*\".*$")
  file(STRINGS "${Tesseract_INCLUDE_DIRS}/version.h" _Tesseract_VERSION_STRING LIMIT_COUNT 1 REGEX "${_Tesseract_VERSION_REGEX}")
  if(_Tesseract_VERSION_STRING)
    string(REGEX REPLACE "${_Tesseract_VERSION_REGEX}" "\\1.\\2.\\3" Tesseract_VERSION "${_Tesseract_VERSION_STRING}")
  endif()
  unset(_Tesseract_VERSION_REGEX)
  unset(_Tesseract_VERSION_STRING)
endif()

find_library(Tesseract_LIBRARY NAMES tesseract400 tesseract
    HINTS
    /usr/lib
    /usr/local/lib
    /opt/lib
    /opt/local/lib
    "c:/program files/tesseract/lib"
    ${Tesseract_DIR}/lib
)
set(Tesseract_LIBRARIES ${Tesseract_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Tesseract
    REQUIRED_VARS
        Tesseract_INCLUDE_DIR
        Tesseract_LIBRARIES
    VERSION_VAR Tesseract_VERSION
    FAIL_MESSAGE "Try to set Tesseract_DIR or Tesseract_ROOT"
)

add_library(TesseractLib SHARED IMPORTED)
set_target_properties(TesseractLib PROPERTIES
  INTERFACE_COMPILE_DEFINITIONS "Tesseract_DLL"
  INTERFACE_INCLUDE_DIRECTORIES "${Tesseract_INCLUDE_DIR}"
  IMPORTED_IMPLIB ${Tesseract_LIBRARIES}
  VERSION ${Tesseract_VERSION}
)

mark_as_advanced(Tesseract_INCLUDE_DIRS Tesseract_LIBRARIES)
