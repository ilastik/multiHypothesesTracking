cmake_minimum_required(VERSION 3.10)
message( "\nConfiguring bin:" )

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost REQUIRED program_options)

include_directories(
	${Boost_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/include/
)

file(GLOB BIN_SRCS *.cpp)
foreach(src ${BIN_SRCS})
    get_filename_component(bin_name ${src} NAME_WE)
    add_executable(${bin_name} ${src})
    target_link_libraries(${bin_name} multiHypoTracking${SUFFIX} ${Boost_LIBRARIES})
    target_compile_definitions(${bin_name} PRIVATE -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK)
endforeach(src)
