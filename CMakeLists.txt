
cmake_minimum_required(VERSION 2.6)
project(HIGHLOAD)

set(CMAKE_CXX_FLAGS "-Wall -Werror -O2 -g -std=c++11 -pthread")

include_directories(/usr/include)
include_directories(/usr/local/include)
include_directories(src)

#link_directories(/usr/lib64)
link_directories(/usr/lib)
link_directories(/usr/local/lib)

find_package(RapidJSON)

add_subdirectory(src/http)
add_subdirectory(src/tcp)
add_subdirectory(src/db)
add_subdirectory(src/server)
