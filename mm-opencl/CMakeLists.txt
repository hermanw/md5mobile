cmake_minimum_required(VERSION 3.18)
project(mm)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

find_package(Boost 1.75.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(mm mm.cpp decoder.cpp compute.cpp)
target_link_libraries(mm ${OpenCL_LIBRARY} Boost::program_options)