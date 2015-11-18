cmake_minimum_required(VERSION 2.8)
message( "\nConfiguring bin:" )

find_package(HDF5 REQUIRED)
find_package(Opengm REQUIRED)

include_directories(
  ${Opengm_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include/
)

file(GLOB BIN_SRCS *.cpp)
foreach(src ${BIN_SRCS})
    get_filename_component(bin_name ${src} NAME_WE)
    add_executable(${bin_name} ${src})
    target_link_libraries(${bin_name} multiHypoTracking ${HDF5_LIBRARIES})
endforeach(src)
