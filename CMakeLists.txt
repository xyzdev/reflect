cmake_minimum_required(VERSION 2.8.8)
project(xyz_reflect)

add_subdirectory("${PROJECT_SOURCE_DIR}/test")

add_definitions(-Wall -Wold-style-cast -std=c++11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/json.cpp src/reflection.cpp)
#add_executable(reflect ${SOURCE_FILES})
