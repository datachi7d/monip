cmake_minimum_required(VERSION 3.10.0)

 set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
add_subdirectory(lib)

enable_testing()
add_subdirectory(tests)
