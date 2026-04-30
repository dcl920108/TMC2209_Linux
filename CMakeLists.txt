cmake_minimum_required(VERSION 3.16)
project(tmc2209_linux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 REQUIRED)

pybind11_add_module(tmc2209_module
    src/linux_serial.cpp
    src/tmc2209_driver.cpp
    src/trapezoidal.cpp
    src/bindings.cpp
    third_party/janelia/TMC2209.cpp
)

target_include_directories(tmc2209_module PRIVATE
    include/
    third_party/janelia/
)

target_link_libraries(tmc2209_module PRIVATE pigpio pthread)
