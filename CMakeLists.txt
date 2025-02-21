cmake_minimum_required(VERSION 2.8)

# Set the project name
project(HW2)

# Add your CMAKE_C_FLAGS and CMAKE_CXX_FLAGS here
set(CMAKE_C_FLAGS "-std=c11 -Wall -Wextra -Wshadow -Werror")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wshadow -Werror")

# Add our include directory to CMake's search paths
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create library from dyn_array
add_library(dyn_array src/dyn_array.c)

# Create library for process scheduling
add_library(processing_scheduling src/processing_scheduling.c)

# Compile the analysis executable
add_executable(analysis src/analysis.c)

# Link the analysis executable with the required libraries
target_link_libraries(analysis dyn_array processing_scheduling)

# Compile the tester executable
add_executable(${PROJECT_NAME}_test test/tests.cpp)

# Link ${PROJECT_NAME}_test with dyn_array, gtest, pthread, and process scheduling
target_link_libraries(${PROJECT_NAME}_test gtest pthread dyn_array processing_scheduling)
