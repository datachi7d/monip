
set(libmonip_SOURCES serial.c)

add_library(libmonip SHARED ${libmonip_SOURCES})

set_target_properties(libmonip PROPERTIES VERSION ${PROJECT_VERSION})
