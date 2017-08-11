
cmake_minimum_required(VERSION 2.6)
project(HIGHLOAD)

set(CMAKE_CXX_FLAGS "-Wall -Werror -O2 -g -std=c++11")

include_directories(/usr/local/include)
include_directories(src)
link_directories(/usr/local/lib)

add_subdirectory(src/tcp)
add_subdirectory(src/server)
