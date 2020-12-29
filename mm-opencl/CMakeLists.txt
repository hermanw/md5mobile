cmake_minimum_required(VERSION 3.8)
project(mm)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

add_executable(mm mm.cpp)
target_link_libraries(mm ${OpenCL_LIBRARY})