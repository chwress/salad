cmake_minimum_required(VERSION 2.6)
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0009 NEW)
endif (COMMAND cmake_policy)

set(TARGETNAME salad)
set(PROJECTNAME ${TARGETNAME})
project(${PROJECTNAME})

set(${TARGETNAME}_VERSION_MAJOR 0)
set(${TARGETNAME}_VERSION_MINOR 3)
set(${TARGETNAME}_VERSION_PATCH 5)

set(VERSION_STR "${${TARGETNAME}_VERSION_MAJOR}.${${TARGETNAME}_VERSION_MINOR}.${${TARGETNAME}_VERSION_PATCH}")


# dependencies

option(USE_ARCHIVES  "Enable support for loading archives (requires libarchive)" ON)
option(_GNU_SOURCE  "Enable GNU extensions" OFF)

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

set(Z TRUE)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(Z FALSE) 
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")


set(SOURCE_DIR "src/")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_DIR}/config.h)


#code

file(GLOB_RECURSE sources "${SOURCE_DIR}*.c")
file(GLOB_RECURSE headers "${SOURCE_DIR}*.h")
source_group("Source Files" FILES ${sources})
source_group("Header Files" FILES ${headers})

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -std=c99")
endif (NOT MSVC)

add_executable(${TARGETNAME} ${headers} ${sources})
target_link_libraries(${TARGETNAME} ${LibArchive_LIBRARIES})

if (CMAKE_COMPILER_IS_GNUCC)
    find_library(M_LIB m)
    if (M_LIB)
        target_link_libraries(${TARGETNAME} ${LibArchive_LIBRARIES} ${M_LIB})
    endif (M_LIB)
endif (CMAKE_COMPILER_IS_GNUCC)


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
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/${DOC_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${DOC_DIR}
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
