add_library(my_common STATIC
  Profiler.cpp)


target_link_libraries(my_common PUBLIC
  tbb)
