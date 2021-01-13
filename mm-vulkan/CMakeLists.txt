cmake_minimum_required(VERSION 3.8)
project(mm)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Release)

find_package(Vuh REQUIRED)

add_executable(mm mm.cpp decoder.cpp)
target_link_libraries(mm PUBLIC vuh::vuh)
