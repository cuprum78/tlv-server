cmake_minimum_required(VERSION 3.6)
project(server)

file(GLOB sources ./*.cpp)

## define target
#---------------

add_executable(server ${sources})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall")
include_directories(/usr/local/include)
target_link_libraries(server boost_system boost_program_options pthread)
