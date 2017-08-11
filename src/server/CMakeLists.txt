add_executable(server.out main.cpp)

target_link_libraries(server.out
  z
  my_tcp)

install(TARGETS server.out
    RUNTIME DESTINATION bin
)