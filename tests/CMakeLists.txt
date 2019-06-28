include(GoogleTest)

add_subdirectory(gtest)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -g -std=c++11 -Wno-narrowing")

add_executable(monip_test serial.cpp)
target_link_libraries(monip_test libmonip  gtest_main)

gtest_discover_tests(monip_test)
