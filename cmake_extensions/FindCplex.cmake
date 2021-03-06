# This module finds cplex.
#
# User can give CPLEX_ROOT_DIR as a hint stored in the cmake cache.
#
# It sets the following variables:
#  CPLEX_FOUND              - Set to false, or undefined, if cplex isn't found.
#  CPLEX_INCLUDE_DIRS       - include directory
#  CPLEX_LIBRARIES          - library files

set(CPLEX_ROOT_DIR "" CACHE PATH "CPLEX root directory.")

if(WIN32)
  set(CPLEX_WIN_VERSION ${CPLEX_WIN_VERSION} CACHE STRING "CPLEX version to be used.")
  if(NOT CPLEX_ROOT_DIR)
    set(CPLEX_STUDIO_DIR CPLEX_STUDIO_DIR${CPLEX_WIN_VERSION})
    execute_process(COMMAND cmd /C set ${CPLEX_STUDIO_DIR} OUTPUT_VARIABLE CPLEX_STUDIO_DIR_VAR ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(NOT CPLEX_STUDIO_DIR_VAR)
      MESSAGE(FATAL_ERROR "Unable to find CPLEX: environment variable CPLEX_STUDIO_DIR<VERSION> not set.")
    endif()

    STRING(REGEX REPLACE "^CPLEX_STUDIO_DIR" "" CPLEX_STUDIO_DIR_VAR ${CPLEX_STUDIO_DIR_VAR})
    if(NOT CPLEX_WIN_VERSION)
      STRING(REGEX MATCH "^[0-9]+" CPLEX_WIN_VERSION ${CPLEX_STUDIO_DIR_VAR})
    endif()

    STRING(REGEX REPLACE "^[0-9]+=" "" CPLEX_STUDIO_DIR_VAR ${CPLEX_STUDIO_DIR_VAR})
    file(TO_CMAKE_PATH "${CPLEX_STUDIO_DIR_VAR}" CPLEX_ROOT_DIR)

  elseif(NOT CPLEX_WIN_VERSION)
    STRING(REGEX MATCH "[0-9]+$" CPLEX_WIN_VERSION ${CPLEX_ROOT_DIR})
    if(NOT CPLEX_WIN_VERSION)
      message(FATAL_ERROR "Unable to determine CPLEX version number. Specify CPLEX_WIN_VERSION")
    endif()
  endif()

  MESSAGE(STATUS "Found CLPEX version ${CPLEX_WIN_VERSION} at '${CPLEX_ROOT_DIR}'")
  set(CPLEX_WIN_VS_VERSION ${CPLEX_WIN_VS_VERSION} CACHE STRING "Visual Studio Version")

  if (${MSVC_TOOLSET_VERSION} EQUAL "140")
    set(CPLEX_WIN_VS_VERSION 2015)
  elseif (${MSVC_TOOLSET_VERSION} EQUAL "141")
    set(CPLEX_WIN_VS_VERSION 2017)
  elseif (${MSVC_TOOLSET_VERSION} EQUAL "142")
    set(CPLEX_WIN_VS_VERSION 2019)
  else()
    MESSAGE(FATAL_ERROR "CPLEX: unknown Visual Studio version '${MSVC_TOOLSET_VERSION}'.")
  endif()

  # We only use 64 bit builds - if you want to build 32 bit stuff make something nice here, I won't bother
  set(CPLEX_WIN_BITNESS x64)
  set(CPLEX_WIN_BITNESS ${CPLEX_WIN_BITNESS} CACHE STRING "On Windows: x86 or x64 (32bit resp. 64bit)")

  if(NOT CPLEX_WIN_LINKAGE)
    set(CPLEX_WIN_LINKAGE mda CACHE STRING "CPLEX linkage variant on Windows. One of these: mda (dll, release), mdd (dll, debug), mta (static, release), mtd (static, debug)")
  endif(NOT CPLEX_WIN_LINKAGE)

  # now, generate platform string
  set(CPLEX_WIN_PLATFORM "${CPLEX_WIN_BITNESS}_windows_vs${CPLEX_WIN_VS_VERSION}/stat_${CPLEX_WIN_LINKAGE}")
  MESSAGE(STATUS "FindCPLEX: using platform string ${CPLEX_WIN_PLATFORM}")

else()

  set(CPLEX_ROOT_DIR "" CACHE PATH "CPLEX root directory.")
  set(CPLEX_WIN_PLATFORM "")

endif()


FIND_PATH(CPLEX_INCLUDE_DIR
  ilcplex/cplex.h
  HINTS ${CPLEX_ROOT_DIR}/cplex/include
        ${CPLEX_ROOT_DIR}/include
  PATHS ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
  )
message(STATUS "CPLEX Include dir: ${CPLEX_INCLUDE_DIR}")

FIND_PATH(CPLEX_CONCERT_INCLUDE_DIR
  ilconcert/iloenv.h
  HINTS ${CPLEX_ROOT_DIR}/concert/include
        ${CPLEX_ROOT_DIR}/include
  PATHS ENV C_INCLUDE_PATH
        ENV C_PLUS_INCLUDE_PATH
        ENV INCLUDE_PATH
  )
message(STATUS "CONCERT Include dir: ${CPLEX_CONCERT_INCLUDE_DIR}")

FIND_LIBRARY(CPLEX_LIBRARY
  NAMES cplex${CPLEX_WIN_VERSION} cplex${CPLEX_WIN_VERSION}0 cplex
  HINTS ${CPLEX_ROOT_DIR}/cplex/lib/${CPLEX_WIN_PLATFORM} #windows
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_debian4.0_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_sles10_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH #unix
        ENV LD_LIBRARY_PATH #unix
  )
message(STATUS "CPLEX Library: ${CPLEX_LIBRARY}")

FIND_LIBRARY(CPLEX_ILOCPLEX_LIBRARY
  ilocplex
  HINTS ${CPLEX_ROOT_DIR}/cplex/lib/${CPLEX_WIN_PLATFORM} #windows
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_debian4.0_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_sles10_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/cplex/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
  )
message(STATUS "ILOCPLEX Library: ${CPLEX_ILOCPLEX_LIBRARY}")

FIND_LIBRARY(CPLEX_CONCERT_LIBRARY
  concert
  HINTS ${CPLEX_ROOT_DIR}/concert/lib/${CPLEX_WIN_PLATFORM} #windows
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_debian4.0_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_sles10_4.1/static_pic #unix
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_osx/static_pic #osx
        ${CPLEX_ROOT_DIR}/concert/lib/x86-64_darwin/static_pic #osx
  PATHS ENV LIBRARY_PATH
        ENV LD_LIBRARY_PATH
  )
message(STATUS "CONCERT Library: ${CPLEX_CONCERT_LIBRARY}")

if(WIN32)
  set(CPLEX_BIN_DIR CPLEX_STUIDO_BINARIES${CPLEX_WIN_VERSION})
else()
    FIND_PATH(CPLEX_BIN_DIR
      cplex
          HINTS ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_sles10_4.1 #unix
                ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_debian4.0_4.1 #unix
                ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_osx #osx
            ${CPLEX_ROOT_DIR}/cplex/bin/x86-64_darwin #osx
      ENV LIBRARY_PATH
          ENV LD_LIBRARY_PATH
      )
endif()
message(STATUS "CPLEX Bin Dir: ${CPLEX_BIN_DIR}")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CPLEX DEFAULT_MSG
 CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_LIBRARY CPLEX_CONCERT_INCLUDE_DIR)

IF(CPLEX_FOUND)
  SET(CPLEX_INCLUDE_DIRS ${CPLEX_INCLUDE_DIR} ${CPLEX_CONCERT_INCLUDE_DIR})
  SET(CPLEX_LIBRARIES ${CPLEX_CONCERT_LIBRARY} ${CPLEX_ILOCPLEX_LIBRARY} ${CPLEX_LIBRARY} )
  IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(CPLEX_LIBRARIES "${CPLEX_LIBRARIES};m;pthread")
  ENDIF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
ENDIF(CPLEX_FOUND)

MARK_AS_ADVANCED(CPLEX_LIBRARY CPLEX_INCLUDE_DIR CPLEX_ILOCPLEX_LIBRARY CPLEX_CONCERT_INCLUDE_DIR CPLEX_CONCERT_LIBRARY)
