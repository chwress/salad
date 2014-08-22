cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0009 NEW)
endif ()

set(TARGETNAME salad)
set(TAGLINE "Salad - A Content Anomaly Detector based on n-Grams")
set(PROJECTNAME lib${TARGETNAME})
project(${PROJECTNAME} C)

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 5)
set(${TARGETNAME}_VERSION_PATCH 0)


set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")


# dependencies

option(_GNU_SOURCE   "Enable GNU extensions" OFF)

set(STRICT TRUE)


set(HAS_Z_MODIFIER TRUE)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(HAS_Z_MODIFIER FALSE) 
endif ()


set(SALAD_DIR "../../")
set(SOURCE_DIR "./")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/${SALAD_DIR}/src")

set(CONFIG_DIR "${SALAD_DIR}/src")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_DIR}/config.h)


#code

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/")
file(GLOB sources "${SRC}/*.c" "${SRC}/../util/*.c")
file(GLOB headers "${SRC}/*.h" "${SRC}/../util/*.h")

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

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
add_library(${TARGETNAME} ${headers} ${sources})

if (CMAKE_COMPILER_IS_GNUCC)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(${TARGETNAME} ${M_LIB})
    endif ()
endif ()


# documentation

set(DOC_DIR "${SALAD_DIR}/doc/")

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
set(SALAD_ICON "${CMAKE_CURRENT_SOURCE_DIR}/${SALAD_DIR}/res/logo/lettuce.ico")
set(SALAD_TOPBAR "${CMAKE_CURRENT_SOURCE_DIR}/${SALAD_DIR}/res/logo/topbar.bmp")

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


# install

set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/${SOURCE_DIR}"
                      "${PROJECT_BINARY_DIR}/${SOURCE_DIR}")
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
	install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_DIR}/${PROJECTNAME}/man/
		DESTINATION share/man
		FILES_MATCHING PATTERN "salad.h.1")
endif ()

