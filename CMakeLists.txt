cmake_minimum_required(VERSION 3.15)

project(vex_logger)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)



file(GLOB_RECURSE source_list
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

file(GLOB_RECURSE header_list
    "${CMAKE_CURRENT_LIST_DIR}/include/logger/*.h")


add_library(${PROJECT_NAME} SHARED ${header_list} ${source_list})


target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)







