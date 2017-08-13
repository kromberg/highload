add_library(my_http STATIC
  HttpServer.cpp
  StateMachine.cpp)

target_link_libraries(my_http PUBLIC
  my_tcp)