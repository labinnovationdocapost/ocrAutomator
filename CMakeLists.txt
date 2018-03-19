cmake_minimum_required (VERSION 3.0)

project (OcrAutomator)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

message(${CMAKE_CURRENT_LIST_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(GLOB_RECURSE WIN32 RELATIVE ${CMAKE_SOURCE_DIR} "OcrAutomatorWin32/*.cpp" "OcrAutomatorWin32/*.c" "OcrAutomatorWin32/*.h")

##############################################################
# OcrAutomator
##############################################################
set(SHARED_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomator")
set(WIN32_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomatorWin32")

# Headers
file(
    GLOB_RECURSE SHARED_HEADER 
    LIST_DIRECTORIES false
    "${SHARED_ROOT}/*.h*"
)

foreach(_source IN ITEMS ${SHARED_HEADER})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${SHARED_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Header Files\\${_group_path}" FILES "${_source}")
endforeach()

# .c, .cpp, .c*
file(
    GLOB_RECURSE SHARED_CXX 
    LIST_DIRECTORIES false
    "${SHARED_ROOT}/*.c*"
)

foreach(_source IN ITEMS ${SHARED_CXX})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${SHARED_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Source Files\\${_group_path}" FILES "${_source}")
endforeach()

##############################################################
# OcrAutomatorWin32
##############################################################
# Headers

set(WIN32_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomatorWin32")

# Headers
file(
    GLOB_RECURSE WIN32_HEADER 
    LIST_DIRECTORIES false
    "${WIN32_ROOT}/*.h*"
)

foreach(_source IN ITEMS ${WIN32_HEADER})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${WIN32_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Header Files\\${_group_path}" FILES "${_source}")
endforeach()

# .c, .cpp, .c*
file(
    GLOB_RECURSE WIN32_CXX 
    LIST_DIRECTORIES false
    "${WIN32_ROOT}/*.c*"
)

foreach(_source IN ITEMS ${WIN32_CXX})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${WIN32_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Source Files\\${_group_path}" FILES "${_source}")
endforeach()

#ADD_SUBDIRECTORY(mySubDir1)

find_package(Boost 1.66 REQUIRED COMPONENTS filesystem chrono date_time log program_options system thread)


find_package(MuPdf REQUIRED)
find_package(JpegTurbo REQUIRED)
find_package(RapidJson REQUIRED)
find_package(A2ia REQUIRED)
find_package(exiv2 REQUIRED)
find_package(zlib REQUIRED)
find_package(rttr 0.9.5 REQUIRED)
find_package(libarchive 3.3.2 REQUIRED)
find_package(tesseract 4.0 REQUIRED)
find_package(leptonica 1.74 REQUIRED)

include_directories(${Tesseract_INCLUDE_DIRS})

if(MSVC)
	add_definitions(-D_WIN32_WINNT=0x0A01)
endif()
add_definitions(-DLIB_EXPORT)
add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-DDISPLAY)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Includes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Libs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/Includes/Protobuf)
include_directories(${JPEG_INCLUDE_DIR})
include_directories(${JpegTurbo_INCLUDE_DIR})
include_directories(${MuPdf_INCLUDE_DIR})

# Evite les ../../ dans les includes
include_directories(${SHARED_ROOT})

# Library
add_library(OcrAutomatorShared STATIC ${SHARED_HEADER} ${SHARED_CXX})
set_target_properties(OcrAutomatorShared PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/OcrAutomator/"
)

IF (WIN32)
# Executable
add_executable(OcrAutomator ${WIN32} ${SHARED_HEADER})

FILE(TO_NATIVE_PATH "${A2ia_DLL}/*.dll" srcDir)
FILE(TO_NATIVE_PATH $<TARGET_FILE_DIR:OcrAutomator> srcDest)
# Copy dll to output
if(MSVC)
	add_custom_command(TARGET OcrAutomator POST_BUILD COMMAND xcopy /y /d /s ${srcDir} \"$(TargetDir)\")
endif()

target_include_directories(OcrAutomator PUBLIC ${WIN32_ROOT})
endif()
				   
target_link_libraries(OcrAutomatorShared "${CMAKE_CURRENT_BINARY_DIR}/Libs/Win32/Debug/libprotobuf.lib")
target_link_libraries(OcrAutomator OcrAutomatorShared)
target_link_libraries(OcrAutomator ${MuPdf_LIBRARIES})
target_link_libraries(OcrAutomatorShared ${JPEG_LIBRARIES})
target_link_libraries(OcrAutomatorShared ${JpegTurbo_LIBRARY})
target_link_libraries(OcrAutomatorShared A2ialib)
target_link_libraries(OcrAutomatorShared exiv2lib)
target_link_libraries(OcrAutomatorShared debug ${ZLIB_LIBRARY_DEBUG})
target_link_libraries(OcrAutomatorShared optimized ${ZLIB_LIBRARY_RELEASE})
target_link_libraries(OcrAutomatorShared RTTR::Core)
target_link_libraries(OcrAutomatorShared ${Boost_LIBRARIES})
target_link_libraries(OcrAutomatorShared ${LibArchive_LIBRARY})
target_link_libraries(OcrAutomatorShared ${Tesseract_LIBRARIES})
target_link_libraries(OcrAutomatorShared leptonica)

INSTALL(TARGETS OcrAutomator RUNTIME DESTINATION ${PROJECT_BINARY_BIN}/bin)