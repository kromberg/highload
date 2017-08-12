add_library(my_http STATIC HttpServer.cpp)

target_link_libraries(my_http PUBLIC
  my_tcp)