cmake_minimum_required (VERSION 3.0)
option(RUN_TEST "Build all tests." OFF) # Makes boolean 'test' available.

add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
set(PROJECT_NAME OcrAutomator CACHE STRING "")
set(PROJECT_NAME_TEST "${PROJECT_NAME}Test")
project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Test_Data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB_RECURSE TEST RELATIVE ${CMAKE_SOURCE_DIR} "OcrAutomatorTest/*.cpp" "OcrAutomatorTest/*.c" "OcrAutomatorTest/*.h")
file(GLOB_RECURSE WIN32 RELATIVE ${CMAKE_SOURCE_DIR} "OcrAutomatorWin32/*.cpp" "OcrAutomatorWin32/*.c" "OcrAutomatorWin32/*.h")
file(GLOB_RECURSE LINUX RELATIVE ${CMAKE_SOURCE_DIR} "OcrAutomatorLinux/*.cpp" "OcrAutomatorLinux/*.c" "OcrAutomatorLinux/*.h")

##############################################################
# OcrAutomator
##############################################################
set(SHARED_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomator")
set(WIN32_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomatorWin32")
set(LINUX_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomatorLinux")
set(TEST_ROOT "${CMAKE_SOURCE_DIR}/OcrAutomatorTest")

 
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

##############################################################
# OcrAutomatorTest
##############################################################
# Headers
file(
    GLOB_RECURSE TEST_HEADER 
    LIST_DIRECTORIES false
    "${TEST_ROOT}/*.h*"
)

foreach(_source IN ITEMS ${TEST_HEADER})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${TEST_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Header Files\\${_group_path}" FILES "${_source}")
endforeach()

# .c, .cpp, .c*
file(
    GLOB_RECURSE TEST_CXX 
    LIST_DIRECTORIES false
    "${TEST_ROOT}/*.c*"
)
message(${TEST_CXX})

foreach(_source IN ITEMS ${TEST_CXX})
    get_filename_component(_source_path "${_source}" PATH)
    file(RELATIVE_PATH _source_path_rel "${TEST_ROOT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("Source Files\\${_group_path}" FILES "${_source}")
endforeach()

#ADD_SUBDIRECTORY(mySubDir1)

if(RUN_TEST)
	find_package(Boost 1.65 REQUIRED COMPONENTS filesystem chrono date_time log program_options system thread unit_test_framework)
	find_package(CURL REQUIRED)
else()
	find_package(Boost 1.65 REQUIRED COMPONENTS filesystem chrono date_time log program_options system thread)
endif()


find_package(MuPdf 1.12.0 REQUIRED)
find_package(JpegTurbo 1.4.0 REQUIRED)
find_package(RapidJson REQUIRED)
if(MSVC)
	find_package(A2ia REQUIRED)
else()
	find_package(Curses REQUIRED)
endif()
find_package(exiv2 0.25 REQUIRED)
find_package(zlib 1.2.11 REQUIRED)
find_package(rttr 0.9.5 REQUIRED)
find_package(libarchive 3.3.2 REQUIRED)
find_package(tesseract 4.00.00 REQUIRED)
find_package(leptonica 1.74 REQUIRED)


if(MSVC)
	add_definitions(-D_WIN32_WINNT=0x0A01)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()
add_definitions(-DLIB_EXPORT)
add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
add_definitions(-DDISPLAY)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Includes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Libs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${Tesseract_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/Includes/Protobuf)
include_directories(${JPEG_INCLUDE_DIR})
include_directories(${JpegTurbo_INCLUDE_DIR})
include_directories(${MuPdf_INCLUDE_DIR})
include_directories(${RTTR_INCLUDE_DIR})
include_directories(${EXIV2_INCLUDE_DIR})
if(RUN_TEST)
	INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
endif()

# Evite les ../../ dans les includes
include_directories(${SHARED_ROOT})

IF (MSVC)
# Executable
	add_executable(${PROJECT_NAME} ${SHARED_HEADER} ${SHARED_CXX} ${WIN32} )

	FILE(TO_NATIVE_PATH "${A2ia_DLL}/*.dll" srcDir)
	FILE(TO_NATIVE_PATH $<TARGET_FILE_DIR:${PROJECT_NAME}> srcDest)
	# Copy dll to output
	if(MSVC)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND xcopy /y /d /s ${srcDir} \"$(TargetDir)\")
	endif()

	target_include_directories(${PROJECT_NAME} PUBLIC ${WIN32_ROOT})
else()
	include_directories(${CURSES_INCLUDE_DIR})
	add_executable(${PROJECT_NAME} ${LINUX} ${SHARED_HEADER} ${SHARED_CXX})
	target_include_directories(${PROJECT_NAME} PUBLIC ${LINUX_ROOT})
endif()

target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${MY_DEBUG_OPTIONS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${MY_RELEASE_OPTIONS}>")
	   
set(SHARED_LIB ${MuPdf_LIBRARIES} ${JPEG_LIBRARIES} ${JpegTurbo_LIBRARY} ${EXIV2_LIBRARY} ${ZLIB_LIBRARIES} ${RTTR_LIBRARY} ${Boost_LIBRARIES} ${LibArchive_LIBRARY} ${Tesseract_LIBRARIES} ${Leptonica_LIBRARIES})

if(MSVC)
	set(SYSTEM_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/Libs/Win32/Debug/libprotobuf.lib" A2ialib)
else()
	set(SYSTEM_LIBRARY protobuf pthread ${CURSES_LIBRARIES})
endif()
target_link_libraries(${PROJECT_NAME} ${SHARED_LIB})
target_link_libraries(${PROJECT_NAME} ${SYSTEM_LIBRARY})

INSTALL(TARGETS OcrAutomator RUNTIME DESTINATION ${PROJECT_BINARY_BIN}/bin)

if(RUN_TEST)
	ENABLE_TESTING()
	
	list(FILTER SHARED_CXX EXCLUDE REGEX ".*Main.cpp$")
	
IF (MSVC)
	add_executable(${PROJECT_NAME_TEST} ${SHARED_HEADER} ${SHARED_CXX} ${WIN32} ${TEST}  )
	target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${WIN32_ROOT})
else()
	add_executable(${PROJECT_NAME_TEST} ${SHARED_HEADER} ${SHARED_CXX} ${LINUX} ${TEST}  )
	target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${LINUX_ROOT})
endif()
	target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${TEST_ROOT})
	
	target_link_libraries(${PROJECT_NAME_TEST} ${SHARED_LIB})
	target_link_libraries(${PROJECT_NAME_TEST} ${SYSTEM_LIBRARY})
	target_link_libraries(${PROJECT_NAME_TEST} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
	target_link_libraries(${PROJECT_NAME_TEST}  ${CURL_LIBRARIES})
	
	add_test(NAME Tesseract COMMAND "${PROJECT_NAME_TEST}" WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME_TEST}>)
 endif()