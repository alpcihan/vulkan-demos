add_subdirectory(shaders)

set(TARGET_NAME demo-07)

set(${TARGET_NAME}_SOURCES
    ${TARGET_NAME}.cpp
)

set(RESOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(SHADER_DIR ${CMAKE_BINARY_DIR}/${TARGET_NAME}/shaders)

configure_file(./file.hpp.in ./file.hpp)
add_executable(${TARGET_NAME} ${${TARGET_NAME}_SOURCES})

target_include_directories(${TARGET_NAME}
    PUBLIC 
        ./
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${TARGET_NAME}
    PUBLIC
        tga_vulkan
        tga_utils
        ${CMAKE_THREAD_LIBS_INIT}
)
add_dependencies(${TARGET_NAME} demo-07_shaders)

if(WIN32)
    set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif(WIN32)