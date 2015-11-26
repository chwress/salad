cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0009 NEW)
	cmake_policy(SET CMP0011 NEW)
endif ()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)


set(OUTPUTNAME foodstoragebox)
set(PROJECTNAME lib${OUTPUTNAME})
set(TAGLINE "Food Storage Box - A Library for Juicy Data Structures")

project(${PROJECTNAME} C)
set(TARGETNAME "${PROJECTNAME}_target")

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 1)
set(${TARGETNAME}_VERSION_PATCH 0)


set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")



# CONFIGURATION

set(STRICT TRUE)

include(VegetableBed.cmake)
find_veggiebed(BASE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_DIR ".")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")

set(CONFIG_DIR "${SOURCE_DIR}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_DIR}/config.h)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)

add_subdirectory(
	"${BASE_SOURCE_DIR}/lib/util"
	"${CMAKE_CURRENT_BINARY_DIR}/util"
)



# CODE

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/")
file(GLOB sources "${SRC}/*.c" "${SRC}/io/*.c")
file(GLOB headers "${SRC}/*.h" "${SRC}/io/*.h")

list(REMOVE_ITEM sources "${SRC}/config.h")

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if (HAS_PARENT)
	set(${PROJECTNAME}_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
endif ()

add_library("${TARGETNAME}" STATIC ${headers} ${sources})
# ATTENTION: This messes with the library dependencies!
set_target_properties("${TARGETNAME}" PROPERTIES OUTPUT_NAME  ${OUTPUTNAME})



# DEPENDENCIES

if (CMAKE_COMPILER_IS_GNUCC)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(${TARGETNAME} ${M_LIB})
    endif ()
endif ()

# libutil
find_package(libutil REQUIRED)

include_directories("${libutil_INCLUDE_DIRS}")
target_link_libraries(${TARGETNAME} ${libutil_LIBRARIES})
add_dependencies("${TARGETNAME}" "${libutil_TARGET}")



# FLAGS

if (NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -O3")
	if (STRICT)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
	else ()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__STRICT_ANSI__")
	endif ()
endif ()



# DOCUMENTATION

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
	set(DOXYGEN_OUTPUTDIR "${CMAKE_BINARY_DIR}/${DOC_DIR}")
	if (NOT CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
		set(DOXYGEN_OUTPUTDIR "${DOXYGEN_OUTPUTDIR}/${PROJECTNAME}")
	endif ()
	set(DOXYGEN_INPUTDIR "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}")
	set(DOXYGEN_FILE_PATTERN "bloom.h bloom_ex.h hash.h")

	configure_file(${BASE_SOURCE_DIR}/${DOC_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/${DOC_DIR}/Doxyfile @ONLY)

	add_custom_target("${PROJECTNAME}_doc"
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${DOC_DIR}/Doxyfile
		WORKING_DIRECTORY ${BASE_SOURCE_DIR}/${DOC_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM)
	set_target_properties("${PROJECTNAME}_doc" PROPERTIES OUTPUT_NAME "doc")
endif ()



# INSTALL

set(CONF_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/${SOURCE_DIR}"
                      "${PROJECT_SOURCE_DIR}/${SOURCE_DIR}")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}/${PROJECTNAME}-config.cmake"
               @ONLY)

set(CONF_INCLUDE_DIRS "\${\${LIBNAME}_CMAKE_DIR}/include")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECTNAME}-config.cmake"
               @ONLY)

install(TARGETS ${TARGETNAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/bloom.h"
              "${CMAKE_CURRENT_SOURCE_DIR}/bloom_ex.h"
              "${CMAKE_CURRENT_SOURCE_DIR}/hash.h"
        DESTINATION "include/foodstoragebox")

if (UNIX)
	install(DIRECTORY ${BASE_SOURCE_DIR}/${DOC_DIR}/${PROJECTNAME}/man/
		DESTINATION share/man
		FILES_MATCHING PATTERN "bloom.h.1" PATTERN "bloom_ex.h.1" PATTERN "hash.h.1")
endif ()

