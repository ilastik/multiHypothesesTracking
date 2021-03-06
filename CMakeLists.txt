CMAKE_MINIMUM_REQUIRED(VERSION 2.8.6)
PROJECT(multiHypoTracking)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_extensions/)

set ( LINUX FALSE )
if(UNIX AND NOT APPLE)
    set( LINUX TRUE )
endif()

# --------------------------------------------------------------
# dependencies
find_package( Opengm REQUIRED )
find_package(HDF5 REQUIRED)

# --------------------------------------------------------------
# configure optimizer

if(WITH_GUROBI AND WITH_CPLEX)
    message(FATAL_ERROR "PLEASE only specify ONE solver to use. Both Cplex and Gurobi have been specified.")
endif()

if(NOT (WITH_GUROBI OR WITH_CPLEX))
    message(FATAL_ERROR "PLEASE specify ONE solver to use. Neither Cplex nor Gurobi have been specified.")
endif()

if(WITH_GUROBI)
    find_package( GUROBI )
    if(GUROBI_FOUND)
        message("Using Gurobi optimizer!")
        set(OPTIMIZER_INCLUDE_DIRS ${GUROBI_INCLUDE_DIR})
        set(OPTIMIZER_LIBRARIES ${GUROBI_LIBRARY} ${GUROBI_CXX_LIBRARY})
        add_definitions(-DWITH_GUROBI)
        message("GUROBI_FOUND: ${GUROBI_FOUND} - GUROBI_LIBRARY: ${GUROBI_LIBRARY}")
    endif()
endif()

if(WITH_CPLEX)
    find_package( Cplex )
	if(CPLEX_FOUND)
	    set(OPTIMIZER_INCLUDE_DIRS ${CPLEX_INCLUDE_DIRS})
	    set(OPTIMIZER_LIBRARIES ${CPLEX_LIBRARIES})
	    add_definitions(-DWITH_CPLEX)
	    # CPLEX switch to be compatible with STL
		ADD_DEFINITIONS(-DIL_STD)
        # Following flags are required by CPLEX 128:
        # the --no-as-needed ... --as-needed is needed :) because the cplex
        # guys forgot to link against libdl
        message("CPLEX_FOUND: ${CPLEX_FOUND} - CPLEX_LIBRARIES: ${CPLEX_LIBRARIES}")
        if ( LINUX )
            set( CPLEX_LINKER_FLAGS "-Wl,--no-as-needed -ldl -Wl,--as-needed" )
        else()
            set( CPLEX_LINKER_FLAGS "-ldl" )
        endif()

        set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CPLEX_LINKER_FLAGS}" )
    endif()
endif()

# opengm defines: do not include LPDef symbols everywhere
ADD_DEFINITIONS(-DOPENGM_LPDEF_NO_SYMBOLS)

# --------------------------------------------------------------
# other config
OPTION(USE_STRING_IDS "Use std::string as Id datatype, otherwise unsigned int (size_t) is used" OFF)
IF(USE_STRING_IDS)
	ADD_DEFINITIONS(-DUSE_STRING_IDS)
ENDIF()

# build options
set(SUFFIX "" CACHE STRING "Library suffix appended to the library name - which enables having several differently configured libraries in the path")

set(WITH_PYTHON "true" CACHE BOOL "Build python wrapper.")

# --------------------------------------------------------------
# check for C++ 11 support:
if (NOT WIN32)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
            message(SEND_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-local-typedefs")
endif()
# --------------------------------------------------------------
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h*)

include_directories(
	${PROJECT_SOURCE_DIR}/include/
	${Opengm_INCLUDE_DIR} 
	${OPTIMIZER_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
	${HDF5_INCLUDE_DIR}
)

if (WIN32)
	add_library(multiHypoTracking${SUFFIX} STATIC ${LIB_SOURCES} ${HEADERS})
else()
	add_library(multiHypoTracking${SUFFIX} SHARED ${LIB_SOURCES} ${HEADERS})
endif()
target_link_libraries(multiHypoTracking${SUFFIX} ${OPTIMIZER_LIBRARIES})

# installation
install(TARGETS multiHypoTracking${SUFFIX} 
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

# --------------------------------------------------------------

add_subdirectory(bin)

if(WITH_PYTHON)
  add_subdirectory(python)
endif()

# --------------------------------------------------------------
enable_testing()
add_subdirectory(test)
