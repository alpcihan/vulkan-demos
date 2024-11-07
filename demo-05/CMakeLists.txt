add_subdirectory(gpro)
add_subdirectory(shaders)

set(TARGET_NAME demo-05)

set(${TARGET_NAME}_SOURCES
    ${TARGET_NAME}.cpp
)

add_executable(${TARGET_NAME} ${${TARGET_NAME}_SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC gpro_${TARGET_NAME}_lib)
add_dependencies(${TARGET_NAME} ${TARGET_NAME}_shaders)

if(WIN32)
    set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif(WIN32)

