cmake_minimum_required(VERSION 2.8)
message( "\nConfiguring bin:" )

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
endforeach(src)
