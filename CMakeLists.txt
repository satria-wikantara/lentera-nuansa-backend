# CMakeLists.txt for the C++ project

# Specify the minimum version of CMake
cmake_minimum_required(VERSION 3.15)

# Set the project name
set(PROJECT_NAME lentera-nuances-backend)

# Set the project
project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the source directory
set(SOURCE_DIR src)

# Find all source files
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")

# Specify output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)

# Add the executable
add_executable(${PROJECT_NAME} ${SOURCES}
        include/chat_server_handler.h
        src/chat_server_handler.cpp
        src/app.cpp
        include/chat_server.h
        include/global_config.h
        src/global_config.cpp)

# Optionally, you can add include directories if needed
# target_include_directories(${PROJECT_NAME} PRIVATE include)

# Optionally, you can link libraries if needed
# target_link_libraries(${PROJECT_NAME} some_library)

# Find Boost
find_package(Boost REQUIRED COMPONENTS program_options)

# Link Boost.Program_options
target_link_libraries(${PROJECT_NAME} PRIVATE 
    Boost::program_options
)

# Custom command to create the build directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add a custom command to run the program after building
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E echo "Running ${PROJECT_NAME}..."
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
)

# Add a custom command to clean the build directory
add_custom_target(clean_build
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning build directory..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/bin
    DEPENDS ${PROJECT_NAME}
)
