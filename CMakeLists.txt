cmake_minimum_required(VERSION 3.25)
project(vietoris_rips CXX)

set(CMAKE_CXX_STANDARD 26)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib/eigen")

add_executable(vietoris_rips src/vietoris_rips.cpp)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
target_compile_options(vietoris_rips PUBLIC -stdlib=libc++ -freflection-latest -fcolor-diagnostics)
target_link_libraries(vietoris_rips Eigen3::Eigen)

