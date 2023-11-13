cmake_minimum_required(VERSION 2.8)
message( "\nConfiguring python wrapper:" )

find_package( PythonInterp REQUIRED )
# dependencies; python major, minor version seem to be required with boost>=1.68 on win
# disclaimer, did not try other python versions
find_package( Boost REQUIRED COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR} serialization )

if(WIN32)
  GET_FILENAME_COMPONENT(PYTHON_PREFIX "${PYTHON_EXECUTABLE}" PATH)
  message(STATUS "PYTHON_PREFIX ${PYTHON_PREFIX}")
  find_library(PYTHON_LIBRARY
        NAMES python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}.lib
        HINTS "${PYTHON_PREFIX}/libs"
        NO_DEFAULT_PATH)
  set(PYTHON_LIBRARIES "${PYTHON_LIBRARY}" CACHE FILEPATH "the Python library")
  
  find_path(PYTHON_INCLUDE_DIRS
        NAMES Python.h
        HINTS "${PYTHON_PREFIX}/include"
        NO_DEFAULT_PATH)
  set(PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}" CACHE PATH "the Python include directory")
else()
  find_package( PythonLibs REQUIRED )
endif()

message("PYTHON_INCLUDE_DIRS: ${PYTHON_INCLUDE_DIRS}")

# include external headers as system includes so we do not have to cope with their warnings
include_directories( SYSTEM ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} include_directories(${PROJECT_SOURCE_DIR}/python/))

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pymultiHypoTracking.cpp.cmake ${CMAKE_CURRENT_BINARY_DIR}/pymultiHypoTracking.cpp @ONLY)

# pymultiHypoTracking
set(PYMULTIHYPOTRACKING_SRCS ${CMAKE_CURRENT_BINARY_DIR}/pymultiHypoTracking.cpp pythonmodel.cpp)

add_library(pymultiHypoTracking${SUFFIX} SHARED ${PYMULTIHYPOTRACKING_SRCS})#
# building with the "new" conda compilers resulted in strange test errors
# like: TypeError: Expecting an object of type dict; got an object of type dict instead
# Solution is not to link against python during the build
# See: https://github.com/casacore/python-casacore/issues/144
if(APPLE)
  target_link_libraries(pymultiHypoTracking${SUFFIX} multiHypoTracking${SUFFIX} ${Boost_LIBRARIES} )
else()
  target_link_libraries(pymultiHypoTracking${SUFFIX} multiHypoTracking${SUFFIX} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )
endif()

if(WIN32)
  set_target_properties(pymultiHypoTracking${SUFFIX} PROPERTIES PREFIX "" OUTPUT_NAME "multiHypoTracking${SUFFIX}" SUFFIX ".pyd")
else()
  set_target_properties(pymultiHypoTracking${SUFFIX} PROPERTIES PREFIX "" OUTPUT_NAME "multiHypoTracking${SUFFIX}" SUFFIX ".so")
endif()

# install

# this will set the install dir to $PYTHON_SITE_PACKAGES
# make use of ${PYTHON_EXECUTABLE} instead of /usr/bin/python
if(NOT DEFINED PYMULTIHYPOTRACKING_INSTALL_DIR OR PYMULTIHYPOTRACKING_INSTALL_DIR MATCHES "^$")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} -c 
                    "from distutils.sysconfig import *; print(get_python_lib(1))"
                    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(TO_CMAKE_PATH ${PYTHON_SITE_PACKAGES} PYMULTIHYPOTRACKING_INSTALL_DIR)
endif()

set(PYMULTIHYPOTRACKING_INSTALL_DIR ${PYMULTIHYPOTRACKING_INSTALL_DIR} CACHE PATH "where to install the multiHypoTracking Python package" FORCE)

# this is the install path relative to CMAKE_INSTALL_PREFIX,
# use this in INSTALL() commands to get packaging right
file(RELATIVE_PATH PYMULTIHYPOTRACKING_INSTALL_DIR ${CMAKE_INSTALL_PREFIX} ${PYMULTIHYPOTRACKING_INSTALL_DIR})

install(TARGETS pymultiHypoTracking${SUFFIX} 
    RUNTIME DESTINATION ${PYMULTIHYPOTRACKING_INSTALL_DIR}
    LIBRARY DESTINATION ${PYMULTIHYPOTRACKING_INSTALL_DIR})