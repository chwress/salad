cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0009 NEW)
	cmake_policy(SET CMP0011 NEW)
endif ()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

set(OUTPUTNAME "salad")
set(PROJECTNAME "lib${OUTPUTNAME}")
set(TAGLINE "Salad - A Content Anomaly Detector based on n-Grams")

project(${PROJECTNAME} C)
set(TARGETNAME "${PROJECTNAME}_target")

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 5)
set(${TARGETNAME}_VERSION_PATCH 0)

if (TARGET ${TARGETNAME})
	return()
endif ()


set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")



# CONFIGURATION

set(STRICT TRUE)
set(FSB_AS_DEPENDENCY FALSE)

include(VegetableBed.cmake)
find_veggiebed(BASE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
find_veggiebed(BASE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(SOURCE_DIR ".")
set(INCLUDE_DIR "${SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIR}")

set(CONFIG_DIR "${INCLUDE_DIR}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_DIR}/config.h)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)

add_subdirectory(
	"${BASE_SOURCE_DIR}/lib/util"
	"${CMAKE_CURRENT_BINARY_DIR}/util"
)

if (FSB_AS_DEPENDENCY)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/container")
endif ()



# CODE

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/")
if (FSB_AS_DEPENDENCY)
	file(GLOB sources "${SRC}/*.c")
	file(GLOB headers "${SRC}/*.h")
else ()
	file(GLOB sources "${SRC}/*.c" "${SRC}/container/*.c")
	file(GLOB headers "${SRC}/*.h" "${SRC}/container*.h")
endif ()

source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if (HAS_PARENT)
	set(${PROJECTNAME}_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
endif ()

add_library("${TARGETNAME}" ${headers} ${sources})
# ATTENTION: This messes with the library dependencies!
set_target_properties("${TARGETNAME}" PROPERTIES OUTPUT_NAME  ${OUTPUTNAME})



# DEPENDENCIES

# libfootstoragebox
if (FSB_AS_DEPENDENCY)
	find_package(libfoodstoragebox REQUIRED)

	include_directories("${libfoodstoragebox_INCLUDE_DIRS}")
	target_link_libraries(${TARGETNAME} ${libfoodstoragebox_LIBRARIES})
	add_dependencies("${TARGETNAME}" "${libfoodstoragebox_TARGET}")
endif ()

# libutil
find_package(libutil REQUIRED)

include_directories("${libutil_INCLUDE_DIRS}")
target_link_libraries(${TARGETNAME} ${libutil_LIBRARIES})
add_dependencies("${TARGETNAME}" "${libutil_TARGET}")


# libm
if (CMAKE_COMPILER_IS_GNUCC)
	find_library(M_LIB m)
	if (M_LIB)
		target_link_libraries(${TARGETNAME} ${M_LIB})
	endif ()
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
	set(DOXYGEN_FILE_PATTERN "salad.h common.h")
	configure_file(${BASE_SOURCE_DIR}/${DOC_DIR}/Doxyfile.in ${BASE_BINARY_DIR}/${DOC_DIR}/Doxyfile @ONLY)

	add_custom_target("${PROJECTNAME}_doc"
		${DOXYGEN_EXECUTABLE} ${BASE_BINARY_DIR}/${DOC_DIR}/Doxyfile
		WORKING_DIRECTORY ${BASE_SOURCE_DIR}/${DOC_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM)
	set_target_properties("${PROJECTNAME}_doc" PROPERTIES OUTPUT_NAME "doc")
endif ()


# PACKAGING

set(SALAD_VENDOR "mlsec.org")
set(SALAD_AUTHOR "Christian Wressnegger")
set(SALAD_URL    "http://mlsec.org/salad")
set(SALAD_MAIL   "christian@mlsec.org")
set(SALAD_ICON "${BASE_SOURCE_DIR}/res/logo/lettuce.ico")
set(SALAD_TOPBAR "${BASE_SOURCE_DIR}/res/logo/topbar.bmp")

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
		set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
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


# INSTALL

get_property(CONF_LOCATION TARGET ${TARGETNAME} PROPERTY LOCATION)
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/${SOURCE_DIR}/../"
                      "${PROJECT_BINARY_DIR}/${SOURCE_DIR}/../")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}/${PROJECTNAME}-config.cmake"
               @ONLY)

set(CONF_INCLUDE_DIRS "\${\${LIBNAME}_CMAKE_DIR}/include")
configure_file(${PROJECTNAME}-config.cmake.in
               "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECTNAME}-config.cmake"
               @ONLY)

install(TARGETS ${TARGETNAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/salad.h" "${CMAKE_CURRENT_SOURCE_DIR}/common.h" DESTINATION "include/salad")

if (UNIX)
	install(DIRECTORY ${BASE_SOURCE_DIR}/${PROJECTNAME}/man/
		DESTINATION share/man
		FILES_MATCHING PATTERN "salad.h.1")
endif ()

