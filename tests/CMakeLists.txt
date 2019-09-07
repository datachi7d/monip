include(GoogleTest)

add_subdirectory(gtest)

set(TEST_SOURCES
    support/process.cc
    test_serial.cc
)

add_executable(monip_test ${TEST_SOURCES})
target_link_libraries(monip_test libmonip gtest_main)
target_include_directories(monip_test PRIVATE support)

gtest_discover_tests(monip_test)

find_program(SOCAT_CMD socat)
execute_process(COMMAND ${SOCAT_CMD} -V 
                RESULT_VARIABLE SOCAT_RESULT
                OUTPUT_QUIET)

if(NOT SOCAT_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not find socat, please install socat")
endif()