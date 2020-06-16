cmake_minimum_required(VERSION 2.6)
project(mm)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

add_executable(mm mm.cpp)
target_link_libraries(mm ${OpenCL_LIBRARY})

set(CMAKE_BUILD_TYPE Release)