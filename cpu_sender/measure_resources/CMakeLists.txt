cmake_minimum_required(VERSION 3.7)
project(MeasureResources)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

add_executable(MeasureResources main.cpp)
