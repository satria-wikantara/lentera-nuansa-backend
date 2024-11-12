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

# Enable testing
enable_testing()

# Add the source directory
set(SOURCE_DIR src)

# Find all source files
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")

# Create a list of source files excluding main.cpp for the library
set(LIB_SOURCES
        src/chat_server_handler.cpp
        src/app.cpp
        src/global_config.cpp
        src/init.cpp
)

# Specify output directories for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)

# Create a library target for the source files
add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES})

# Add the main executable
add_executable(${PROJECT_NAME}
        src/main.cpp
)

# Link the main executable with the library
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

# Find Google Test
find_package(GTest REQUIRED)
if(NOT GTest_FOUND)
    message(FATAL_ERROR "Google Test not found")
endif()

# Add test executable
add_executable(${PROJECT_NAME}_test
        tests/chat_server_handler_test.cpp
)

# Link test executable with Google Test, Google Mock and our library
target_link_libraries(${PROJECT_NAME}_test
        PRIVATE
        GTest::GTest
        GTest::Main
        GTest::gmock
        GTest::gmock_main
        ${PROJECT_NAME}_lib
)

# Add test to CTest
add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)

# Custom command to copy config.yml to the build directory based on the build configuration
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.yml
        $<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>/config.yml
)

# Include directories for both targets
target_include_directories(${PROJECT_NAME}_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME}_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find Boost
find_package(Boost REQUIRED COMPONENTS
        program_options
        log
        log_setup
)

# Add yaml-cpp
find_package(yaml-cpp REQUIRED)
if(NOT yaml-cpp_FOUND)
    message(FATAL_ERROR "yaml-cpp not found")
endif()

# Link Boost Libraries to both library and executable
target_link_libraries(${PROJECT_NAME}_lib PUBLIC
        Boost::program_options
        Boost::log
        Boost::log_setup
        yaml-cpp::yaml-cpp
)

# If using Homebrew on MacOS, you might need to add this
if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    # For Apple Silicon (M1/M2) machines, add the Homebrew path
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(GTEST_ROOT "/opt/homebrew/opt/googletest")
        include_directories("/opt/homebrew/include")
        link_directories("/opt/homebrew/lib")
    endif()
endif()

# On Unix-like systems, we need to link against pthread
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC Threads::Threads)
endif()

# Custom command to create the build directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add a custom command to run the program after building
add_custom_target(run
        COMMAND ${CMAKE_COMMAND} -E echo "Running ${PROJECT_NAME}..."
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
)

# Add a custom target to run tests
add_custom_target(run_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests..."
        COMMAND $<TARGET_FILE:${PROJECT_NAME}_test>
        DEPENDS ${PROJECT_NAME}_test
)

# Add a custom command to clean the build directory
add_custom_target(clean_build
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning build directory..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/bin
        DEPENDS ${PROJECT_NAME}
)