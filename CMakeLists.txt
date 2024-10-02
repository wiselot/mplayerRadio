cmake_minimum_required(VERSION 3.10)
project(mplayerRadio VERSION 1.0)

include_directories(${PROJECT_SOURCE_DIR}/src/include)

add_subdirectory(src)
add_subdirectory(test)