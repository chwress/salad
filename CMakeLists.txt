cmake_minimum_required(VERSION 2.6)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0009 NEW)
endif (COMMAND cmake_policy)

set(TARGETNAME salad)
set(PROJECTNAME ${TARGETNAME})
project(${PROJECTNAME})

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 4)
set(${TARGETNAME}_VERSION_PATCH 1)

set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")


# dependencies

option(USE_ARCHIVES "Enable support for loading archives (requires libarchive)" OFF)
option(USE_NETWORK  "Enable support for reading data from network interfaces (requires libnids)" OFF)
option(ALLOW_LIVE_TRAINING
         "Enable support for reading data from network interfaces (requires libnids)" OFF)
option(_GNU_SOURCE  "Enable GNU extensions" OFF)
option(TEST_SALAD   "Enable the ability to run the integrated unit tests" OFF)

if (USE_ARCHIVES)
    find_package(LibArchive 2.70)    
    set(USE_ARCHIVES ${LibArchive_FOUND})
endif (USE_ARCHIVES)

if (USE_ARCHIVES)
    if (${LibArchive_VERSION} VERSION_LESS "3")
        set(LIBARCHIVE2 1)
    endif (${LibArchive_VERSION} VERSION_LESS "3")

    if (LibArchive_FOUND)
        include_directories(${LibArchive_INCLUDE_DIRS})
    endif (LibArchive_FOUND)
endif (USE_ARCHIVES)


if (USE_NETWORK)
    find_library(NIDS_LIB nids)
    if (NOT NIDS_LIB)
    	set(USE_NETWORK FALSE)
    endif (NOT NIDS_LIB)
    find_library(PCAP_LIB pcap)
    if (NOT PCAP_LIB)
    	set(USE_NETWORK FALSE)
    endif (NOT PCAP_LIB)
endif (USE_NETWORK)

set(STRICT TRUE)
if (USE_NETWORK)
    set(STRICT FALSE)
endif (USE_NETWORK)


set(HAS_Z_MODIFIER TRUE)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(HAS_Z_MODIFIER FALSE) 
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")


if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(TEST_SALAD TRUE)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")

set(TEST_RESOURCES "")
if (TEST_SALAD)
	set(TEST_RESOURCES "#define TEST_SRC \"${CMAKE_CURRENT_SOURCE_DIR}/\"\n")
	message(STATUS "${TEST_RESOURCES}")
endif (TEST_SALAD)

set(SOURCE_DIR "src/")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h)


#code

file(GLOB_RECURSE sources "${SOURCE_DIR}*.c")
file(GLOB_RECURSE headers "${SOURCE_DIR}*.h")

if (NOT TEST_SALAD)
	message(STATUS "${sources}")
	foreach (f ${sources})
		if (${f} MATCHES ".*/test/.*")
			message(STATUS "${f}")
			list(REMOVE_ITEM sources ${f})
		endif (${f} MATCHES ".*/test/.*")
	endforeach (f ${sources})
endif (NOT TEST_SALAD)

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})


if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")
    if (STRICT)
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
    else (STRICT)
    	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__STRICT_ANSI__")
    endif (STRICT)
endif (NOT MSVC)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_executable(${TARGETNAME} ${headers} ${sources})
target_link_libraries(${TARGETNAME} ${LibArchive_LIBRARIES})

if (CMAKE_COMPILER_IS_GNUCC)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(${TARGETNAME} ${M_LIB})
    endif (M_LIB)
endif (CMAKE_COMPILER_IS_GNUCC)

if (NIDS_LIB)
    target_link_libraries(${TARGETNAME} ${NIDS_LIB})
endif (NIDS_LIB)

if (PCAP_LIB)
    target_link_libraries(${TARGETNAME} ${PCAP_LIB})
endif (PCAP_LIB)


# documentation

set(DOC_DIR "doc/")

find_package(Doxygen)
if (${DOXYGEN_VERSION})
    string(REGEX REPLACE "^([0-9]+\\.[0-9]+\\.[0-9]+)(.[0-9]+)" "\\1" DOXYGEN_VERSION_ ${DOXYGEN_VERSION}) 

    if (${DOXYGEN_VERSION_} STREQUAL "1.8.3")
        message(STATUS "Doxygen-${DOXYGEN_VERSION} contains a bug that breaks the generation of Salad's documentation. Sorry, no can do!")
        unset(DOXYGEN_FOUND)
    endif (${DOXYGEN_VERSION_} STREQUAL "1.8.3")
endif (${DOXYGEN_VERSION})

if (DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile.in ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif (DOXYGEN_FOUND)


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
install(PROGRAMS ${aliases} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/${DOC_DIR}/man/
        DESTINATION share/man
        FILES_MATCHING PATTERN "salad*"
        PATTERN ".git" EXCLUDE)
