add_executable(server.out main.cpp)

target_link_libraries(server.out
  z
  zip
  tcmalloc
  my_http
  my_tcp
  my_db)

install(TARGETS server.out
    RUNTIME DESTINATION bin
)