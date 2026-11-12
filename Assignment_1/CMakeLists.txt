cmake_minimum_required(VERSION 3.5)
project(assignment1)

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()


# Project sources
add_executable(${PROJECT_NAME} src/main.cpp)
find_package(Eigen3 3.3 QUIET NO_MODULE HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../ext/eigen")

if(TARGET Eigen3::Eigen)
    target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)
else()
    target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../ext/eigen")
endif()

# Use C++11 version of the standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# Place the output binary at the root of the build folder
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Folder where data files are stored (meshes & stuff)
set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/")
target_compile_definitions(${PROJECT_NAME} PUBLIC -DDATA_DIR=\"${DATA_DIR}\")