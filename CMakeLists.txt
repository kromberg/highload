
cmake_minimum_required(VERSION 2.6)
project(HIGHLOAD)

set(TBBROOT /root/tbb-2017_U7/)
set(TBBLIB_ROOT /root/tbb-2017_U7/build/linux_intel64_gcc_cc4.8.5_libc2.17_kernel4.9.36_release)

set(CMAKE_CXX_FLAGS "-Wall -Werror -O2 -g -std=c++11 -pthread")

include_directories(/usr/include)
include_directories(/usr/local/include)
include_directories(/usr/local/lib/libzip/include)
include_directories(${TBBROOT}/include)
include_directories(src)

link_directories(/usr/lib64)
link_directories(/usr/lib)
link_directories(/usr/local/lib)
link_directories(${TBBLIB_ROOT})

find_package(RapidJSON)

add_subdirectory(src/http)
add_subdirectory(src/tcp)
add_subdirectory(src/db)
add_subdirectory(src/server)
