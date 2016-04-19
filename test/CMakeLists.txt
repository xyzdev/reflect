cmake_minimum_required(VERSION 2.8.8)
project(xyz_reflect_test)

include_directories("${PROJECT_BINARY_DIR}/src")

include_directories("${PROJECT_SOURCE_DIR}/../src")

file(GLOB_RECURSE SOURCE_FILES
    "src/*.hpp"
    "src/*.cpp"

    "../src/json.cpp"
    "../src/reflection.cpp"
)

add_definitions(-Wall -Wold-style-cast -std=c++11)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
#target_link_libraries(${PROJECT_NAME})
