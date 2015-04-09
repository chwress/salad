cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0009 NEW)
endif ()

set(TARGETNAME salad)
set(TAGLINE "Salad - A Content Anomaly Detector based on n-Grams")
set(PROJECTNAME ${TARGETNAME})
project(${PROJECTNAME} C)

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 5)
set(${TARGETNAME}_VERSION_PATCH 0)


set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")


if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(USE_REGEX_DEFAULT "ON")
else ()
	set(USE_REGEX_DEFAULT "OFF")
endif ()


# dependencies
option(USE_ARCHIVES  "Enable support for loading archives (requires libarchive)" OFF)
option(USE_REGEX_FILTER
             "Enable support for grouped inputs (only for archives)" ${USE_REGEX_DEFAULT})
option(GROUPED_INPUT "Enable support for grouped inputs (only for archives)" ON)
option(USE_NETWORK   "Enable support for reading data from network interfaces (requires libnids)" OFF)
option(ALLOW_LIVE_TRAINING
         "Enable support for reading data from network interfaces (requires libnids)" OFF)
option(_XOPEN_SOURCE_7   "Enable X/Open 7 extendions (incorporating POSIX 2008)" OFF)
option(TEST_SALAD    "Enable the ability to run the integrated unit tests" OFF)
option(USE_COLORS    "Enable the use of colors for salad's output" ON)

if (USE_ARCHIVES)
    find_package(LibArchive 2.70)    
    set(USE_ARCHIVES ${LibArchive_FOUND})
endif ()

if (USE_ARCHIVES)
    if (${LibArchive_VERSION} VERSION_LESS "3")
        set(LIBARCHIVE2 1)
    endif (${LibArchive_VERSION} VERSION_LESS "3")

    include_directories(${LibArchive_INCLUDE_DIRS})
else ()
    set(GROUPED_INPUT FALSE)
endif ()


if (USE_NETWORK)
    find_library(NIDS_LIB nids)
    if (NOT NIDS_LIB)
	message(STATUS "Unable to locate libnids. Disable network usage!")
    	set(USE_NETWORK FALSE)
    endif ()
    find_library(PCAP_LIB pcap)
    if (NOT PCAP_LIB)
	message(STATUS "Unable to locate libpcap. Disable network usage!")
    	set(USE_NETWORK FALSE)
    endif ()
endif ()

set(STRICT TRUE)
if (USE_NETWORK)
    set(STRICT FALSE)
endif (USE_NETWORK)


set(HAS_Z_MODIFIER TRUE)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(HAS_Z_MODIFIER FALSE)
	if (USE_REGEX_FILTER)
		message(STATUS "Right now it is not possible to use the regex filter on Windows")
		set(USE_REGEX_FILTER FALSE)
	endif ()
endif ()


if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(TEST_SALAD TRUE)
endif ()

set(TEST_RESOURCES "")
if (TEST_SALAD)
	set(TEST_RESOURCES "#define TEST_SRC \"${CMAKE_CURRENT_SOURCE_DIR}/\"\n")
endif ()

if (USE_NETWORK AND _XOPEN_SOURCE)
	message(STATUS "Reject the usage of X/Open in favor of the network support.")
	set(_XOPEN_SOURCE FALSE)
endif ()

if (_XOPEN_SOURCE)
	set(_XOPEN_SOURCE "700")
endif ()

set(SOURCE_DIR "src/")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h)


#code

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/")
file(GLOB sources "${SRC}/*.c" "${SRC}/util/*.c" "${SRC}/salad/*.c" "${SRC}/salad/container/*.c")
file(GLOB headers "${SRC}/*.h" "${SRC}/util/*.h" "${SRC}/salad/*.h" "${SRC}/salad/container/*.h")

if (TEST_SALAD)
	file(GLOB test_sources "${SRC}/test/*.c")
	file(GLOB test_headers "${SRC}/test/*.h")

	set(sources ${sources} ${test_sources})
	set(headers ${headers} ${test_headers})
endif ()

if (_XOPEN_SOURCE_7)
	list(REMOVE_ITEM sources "${SRC}/util/getline.c")
endif ()

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})


if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -O3")
    if (STRICT)
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
    else ()
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__STRICT_ANSI__")
    endif ()
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/salad")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(${TARGETNAME} ${headers} ${sources})
target_link_libraries(${TARGETNAME} ${LibArchive_LIBRARIES})

if (CMAKE_COMPILER_IS_GNUCC)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(${TARGETNAME} ${M_LIB})
    endif ()
endif ()

if (NIDS_LIB)
    target_link_libraries(${TARGETNAME} ${NIDS_LIB})
endif ()

if (PCAP_LIB)
    target_link_libraries(${TARGETNAME} ${PCAP_LIB})
endif ()


# documentation

set(DOC_DIR "doc/")

find_package(Doxygen)
if (${DOXYGEN_VERSION})
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+)(.[0-9]+)" "\\1" DOXYGEN_VERSION_ ${DOXYGEN_VERSION}) 

    if (${DOXYGEN_VERSION_} STREQUAL "1.8.3")
        message(STATUS "Doxygen-${DOXYGEN_VERSION} contains a bug that breaks the generation of Salad's documentation. Sorry, no can do!")
        unset(DOXYGEN_FOUND)
    endif ()
endif ()

if (DOXYGEN_FOUND)
    set(DOXYGEN_FILE_PATTERN "main.h")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif ()


# packaging

set(SALAD_VENDOR "mlsec.org")
set(SALAD_AUTHOR "Christian Wressnegger")
set(SALAD_URL    "http://mlsec.org/salad")
set(SALAD_MAIL   "christian@mlsec.org")
set(SALAD_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/logo/lettuce.ico")
set(SALAD_TOPBAR "${CMAKE_CURRENT_SOURCE_DIR}/res/logo/topbar.bmp")

if (${CMAKE_VERSION} VERSION_GREATER 2.8.7)
	include(InstallRequiredSystemLibraries)

	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${TAGLINE}")
	set(CPACK_PACKAGE_VENDOR "${SALAD_VENDOR}")
	set(CPACK_PACKAGE_CONTACT "${SALAD_AUTHOR} <${SALAD_MAIL}>")
	set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	set(CPACK_PACKAGE_VERSION_MAJOR "${${TARGETNAME}_VERSION_MAJOR}")
	set(CPACK_PACKAGE_VERSION_MINOR "${${TARGETNAME}_VERSION_MINOR}")
	set(CPACK_PACKAGE_VERSION_PATCH "${${TARGETNAME}_VERSION_PATCH}")

	set(CPACK_PACKAGE_EXECUTABLES "${TARGETNAME}" "${TAGLINE}")

	if (NOT CMAKE_SYSTEM_PROCESSOR)
		include(TargetArch.cmake)
		target_architecture(${CMAKE_SYSTEM_PROCESSOR})
	endif ()

	set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
	string(TOLOWER "${CPACK_SYSTEM_NAME}" CPACK_SYSTEM_NAME)

	set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
	if (WIN32)
		list(APPEND CPACK_GENERATOR "ZIP" "NSIS")

		set(CPACK_NSIS_PACKAGE_NAME "Salad")
		set(CPACK_NSIS_DISPLAY_NAME "${TAGLINE}")
		#string(REPLACE "/" "\\\\" CPACK_NSIS_MUI_ICON "${SALAD_ICON}")
		string(REPLACE "/" "\\\\" CPACK_PACKAGE_ICON  "${SALAD_TOPBAR}")
		set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${TARGETNAME}.exe" )
		set(CPACK_NSIS_HELP_LINK "${SALAD_URL}")
		set(CPACK_NSIS_URL_INFO_ABOUT "${SALAD_URL}")
		set(CPACK_NSIS_CONTACT "${SALAD_MAIL}")
		set(CPACK_NSIS_MODIFY_PATH ON)
		set(CPACK_PACKAGE_INSTALL_DIRECTORY "salad")
	else()
		list(APPEND CPACK_GENERATOR "TGZ" "STGZ")

		set(CPACK_STRIP_FILES "bin/${TARGETNAME}")
		set(CPACK_SOURCE_STRIP_FILES "")
	endif()

	if (UNIX AND EXISTS /usr/bin/dpkg)
		list(APPEND CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS "libarchive13 (>= 2.70), libnids1.21 (>= 1.23)")
		set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
		set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
		set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${SALAD_URL}")

		execute_process(COMMAND dpkg --print-architecture
			OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)

		file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/README.md" CPACK_DEBIAN_PACKAGE_DESCRIPTION)
		string(REGEX REPLACE "([^\\]|^);" "\\1\n" CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_DEBIAN_PACKAGE_DESCRIPTION}")
	endif()

	if (UNIX AND EXISTS /usr/bin/rpmbuild)
		list(APPEND CPACK_GENERATOR "RPM")
		set(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
		set(CPACK_RPM_PACKAGE_LICENSE "GNU GENERAL PUBLIC LICENSE 3")
		set(CPACK_RPM_PACKAGE_URL ${SALAD_URL})
	endif()

	include(CPack)
endif ()


# install

set(BIN_DIR "bin/")

foreach(mode "train" "predict" "stats" "inspect")
    set(SALAD_MODE ${mode})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${BIN_DIR}/salad-x.sh.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${BIN_DIR}/salad-${mode})
endforeach(mode)

file(GLOB aliases "${CMAKE_CURRENT_BINARY_DIR}/${BIN_DIR}/salad-*")
list(REMOVE_ITEM aliases "${CMAKE_CURRENT_BINARY_DIR}/${BIN_DIR}/salad-x.sh.in")


install(TARGETS ${TARGETNAME} RUNTIME DESTINATION bin)

if (UNIX)
	install(PROGRAMS ${aliases} DESTINATION bin)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/${DOC_DIR}/${PROJECTNAME}/man/
		DESTINATION share/man
	        FILES_MATCHING PATTERN "salad.1" PATTERN "salad-*.1")
endif ()


