cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0009 NEW)
	cmake_policy(SET CMP0011 NEW)
endif ()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(PROJECTNAME ctest)
set(TAGLINE "a unit test framework for software written in C")

project(${PROJECTNAME} C)



# CONFIGURATION

set(INCLUDE_DIR "include/")
get_directory_property(HAS_PARENT PARENT_DIRECTORY)



# CODE

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if (HAS_PARENT)
	set(${PROJECTNAME}_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
endif ()



# INSTALL

set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/${INCLUDE_DIR}/"
                      "${PROJECT_BINARY_DIR}/${INCLUDE_DIR}/")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}/${PROJECTNAME}-config.cmake"
               @ONLY)

set(CONF_INCLUDE_DIRS "\${\${LIBNAME}_CMAKE_DIR}/${INCLUDE_DIR}")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECTNAME}-config.cmake"
               @ONLY)

