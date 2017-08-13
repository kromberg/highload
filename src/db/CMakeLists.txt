add_library(my_db STATIC
  Storage.cpp
  User.cpp
  Location.cpp
  Visit.cpp)

target_include_directories(my_db PUBLIC
  /usr/local/lib/libzip/include
)

target_link_libraries(my_db PUBLIC
  z
  zip
  tbb)
