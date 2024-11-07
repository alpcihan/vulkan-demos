add_subdirectory(gpro)
add_subdirectory(shaders)

set(TARGET_NAME demo-03)

set(${TARGET_NAME}_SOURCES
    demo-03.cpp
)

add_executable(${TARGET_NAME} ${${TARGET_NAME}_SOURCES})
target_link_libraries(${TARGET_NAME} PUBLIC gpro_demo-03_lib)
add_dependencies(${TARGET_NAME} demo-03_shaders)

if(WIN32)
    set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif(WIN32)

