cmake_minimum_required(VERSION 3.13)
project(gpupro VERSION 1.0 LANGUAGES C CXX DESCRIPTION "Vulkan Playground")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Copy the "model" folder and its contents to the build directory
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/models DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/textures DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/scenes DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/resources)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif(CMAKE_COMPILER_IS_GNUCXX)
if(WIN32)
    add_definitions(-DNOMINMAX)
endif(WIN32)

find_package(Threads REQUIRED)

#####################################################################
### external
#####################################################################
add_subdirectory(external)

#####################################################################
### demos
#####################################################################
add_subdirectory(demo-01)
add_subdirectory(demo-02)
add_subdirectory(demo-03)
add_subdirectory(demo-04)
add_subdirectory(demo-05)
add_subdirectory(demo-06)
add_subdirectory(demo-07)
add_subdirectory(demo-07)