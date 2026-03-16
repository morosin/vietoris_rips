cmake_minimum_required(VERSION 3.25)
project(vietoris_rips CXX)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib/eigen")

add_executable(vietoris_rips src/vietoris_rips.cpp)
target_compile_options(vietoris_rips PUBLIC -freflection-latest -fcolor-diagnostics)
target_link_libraries(vietoris_rips Eigen3::Eigen)

