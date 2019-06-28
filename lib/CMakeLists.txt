set(libmonip_SOURCES 
    serial.c
)

add_library(libmonip SHARED ${libmonip_SOURCES})

target_include_directories(libmonip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(libmonip PROPERTIES 
    VERSION 0.0.1
    SOVERSION 1
    OUTPUT_NAME monip
)

set_property (TARGET libmonip PROPERTY POSITION_INDEPENDANT_CODE ON)
