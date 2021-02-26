cmake_minimum_required(VERSION 3.16)
project(dm)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})

find_package(Boost 1.75.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(dm dm.cpp cfg.cpp decoder.cpp kernel.cpp)
target_link_libraries(dm ${OpenCL_LIBRARY} Boost::program_options)
