idf_component_register(SRCS "temp.c"
                    INCLUDE_DIRS ".")
# I used ESP-IDF’s component-based build system to modularize the firmware, instead of using one big
# file. In here, each component is independent and registered via CMake and compiled with 
# defined dependencies.