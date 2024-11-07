set(DEMO_ID "demo-03")
set(GPRO_LIB_NAME "gpro_${DEMO_ID}_lib")

add_library(${GPRO_LIB_NAME})

configure_file(./include/gpro/file.hpp.in include/gpro/file.hpp)

target_include_directories(${GPRO_LIB_NAME}
    PUBLIC 
        ./include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

file(GLOB_RECURSE GPRO_LIB_SOURCE
    "./src/*"
)

target_sources(${GPRO_LIB_NAME} 
    PUBLIC
        ${GPRO_LIB_SOURCE}
)

target_link_libraries(${GPRO_LIB_NAME} 
    PUBLIC
        tga_vulkan tga_utils ${CMAKE_THREAD_LIBS_INIT}
)