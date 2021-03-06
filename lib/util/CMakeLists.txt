cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0009 NEW)
	cmake_policy(SET CMP0011 NEW)
endif ()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(OUTPUTNAME util)
set(PROJECTNAME lib${OUTPUTNAME})
set(TAGLINE "Yet another utility library")

project(${PROJECTNAME} C)
set(TARGETNAME "${PROJECTNAME}_target")

if (TARGET ${TARGETNAME})
	return()
endif ()


# CONFIGURATION

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(USE_REGEX_DEFAULT "ON")
else ()
	set(USE_REGEX_DEFAULT "OFF")
endif ()

option(USE_ARCHIVES        "Enable support for loading archives (requires libarchive)" OFF)
option(USE_REGEX_FILTER    "Enable support for grouped inputs (only for archives)" ${USE_REGEX_DEFAULT})
option(EXTENDED_METADATA   "Enable the support for extended meta data (filenames, ...)" OFF)
option(GROUPED_INPUT       "Enable support for grouped inputs" OFF)
option(USE_NETWORK         "Enable support for reading data from network interfaces (requires libnids)" OFF)
option(_XOPEN_SOURCE_7     "Enable X/Open 7 extensions (incorporating POSIX 2008)" OFF)


set(HAS_Z_MODIFIER TRUE)
set(HAS_L_MODIFIER TRUE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(HAS_Z_MODIFIER FALSE)
	set(HAS_L_MODIFIER FALSE)
	
	if (USE_COLORS)
		message(WARNING "Cannot use colors on Windows!")
		set(USE_COLORS FALSE)
	endif ()
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows") 
endif ()

if (_XOPEN_SOURCE_7)
	set(_XOPEN_SOURCE "700")
endif ()

if (USE_NETWORK AND _XOPEN_SOURCE)
	message(STATUS "Reject the usage of X/Open in favor of network support.")
	set(_XOPEN_SOURCE FALSE)
endif ()

include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIGENDIAN)

set(SOURCE_DIR "src/")
set(INCLUDE_DIR "include/")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/${INCLUDE_DIR}/util/config.h)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)



# CODE

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/")
set(INC "${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}/")
file(GLOB sources "${SRC}/*.c" "${SRC}/io/*.c")
file(GLOB headers "${SRC}/*.h"  "${SRC}/io/*.h" "${INC}/*.h")

list(REMOVE_ITEM sources "${INC}/config.h")

if (NOT USE_NETWORK)
	list(REMOVE_ITEM sources "${SRC}/io/net.c")
endif ()

if (NOT USE_ARCHIVES)
	list(REMOVE_ITEM sources "${SRC}/archive.c")
	list(REMOVE_ITEM sources "${SRC}/io/archive.c")
endif ()

if (NOT IOTYPE_FILES)
	list(REMOVE_ITEM sources "${SRC}/io/directory.c")
endif ()

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if (HAS_PARENT)
	set(${PROJECTNAME}_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
endif ()

add_library("${TARGETNAME}" ${headers} ${sources})
# ATTENTION: This messes with the library dependencies!
set_target_properties("${TARGETNAME}" PROPERTIES OUTPUT_NAME  ${OUTPUTNAME})

include_directories("${CMAKE_CURRENT_BINARY_DIR}/${INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}")



# DEPENDENCIES

# libarchive
if (USE_ARCHIVES)
	find_package(LibArchive 2.70)    
	set(USE_ARCHIVES ${LibArchive_FOUND})
endif ()

if (USE_ARCHIVES)
	if (${LibArchive_VERSION} VERSION_LESS "3")
		set(LIBARCHIVE2 1)
	endif (${LibArchive_VERSION} VERSION_LESS "3")

	include_directories(${LibArchive_INCLUDE_DIRS})
	target_link_libraries(${TARGETNAME} ${LibArchive_LIBRARIES})
else ()
	set(GROUPED_INPUT FALSE)
endif ()


# libnids
if (USE_NETWORK)
	find_library(NIDS_LIB nids)
	if (NIDS_LIB)
		target_link_libraries(${TARGETNAME} ${NIDS_LIB})
	else ()
		message(STATUS "Unable to locate libnids. Disable network usage!")
		set(USE_NETWORK FALSE)
	endif ()

	find_library(PCAP_LIB pcap)
	if (PCAP_LIB)
		target_link_libraries(${TARGETNAME} ${PCAP_LIB})
	else ()
		message(STATUS "Unable to locate libpcap. Disable network usage!")
		set(USE_NETWORK FALSE)
	endif ()
endif ()


set(STRICT TRUE)
if (USE_NETWORK)
	set(STRICT FALSE)
endif (USE_NETWORK)


# regex stuff
if (USE_REGEX_FILTER AND ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	message(STATUS "Right now it is not possible to use the regex filter on Windows")
	set(USE_REGEX_FILTER FALSE)
endif ()


# libm
find_library(M_LIB m)
if (M_LIB)
	target_link_libraries(${TARGETNAME} ${M_LIB})
endif ()



# FLAGS
if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -O3")
    if (STRICT)
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
    else ()
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__STRICT_ANSI__")
    endif ()
endif ()



# INSTALL

set(CONF_LIBRARIES "${TARGETNAME}")
set(CONF_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/${INCLUDE_DIR}/"
                      "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}/${PROJECTNAME}-config.cmake"
               @ONLY)

set(CONF_INCLUDE_DIRS "\${\${LIBNAME}_CMAKE_DIR}/${INCLUDE_DIR}")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECTNAME}-config.cmake"
               @ONLY)

