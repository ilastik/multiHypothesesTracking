cmake_minimum_required(VERSION 3.10)
message( "\nConfiguring tests:" )

# dependencies
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(Opengm REQUIRED)

include_directories(
	${Boost_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include/
)

# enable dynamic linking with boost test
add_definitions(-DBOOST_TEST_DYN_LINK)

# autodiscover test sources and add tests
file(GLOB TEST_SRCS *.cpp)
foreach(test_src ${TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable( ${test_name} ${test_src} )
    target_link_libraries( ${test_name} multiHypoTracking${SUFFIX} ${Boost_LIBRARIES})
    add_test( ${test_name} ${test_name})
endforeach(test_src)
