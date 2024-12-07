################################################################################
# 1. Basic Project Configuration
################################################################################
cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME nuansa)
project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

################################################################################
# 2. Global Settings
################################################################################
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directory Configuration
set(SOURCE_DIR src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source Files
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

################################################################################
# 3. Platform Detection
################################################################################
if (APPLE)
    set(IS_MACOS TRUE)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(IS_M1_MAC TRUE)
    endif ()
endif ()

################################################################################
# 4. Dependencies
################################################################################
# Core Dependencies
find_package(Boost REQUIRED COMPONENTS program_options log log_setup)
find_package(yaml-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Database Dependencies
if (IS_M1_MAC)
    set(LIBPQXX_PATH "/opt/homebrew/Cellar/libpqxx/7.9.2")
    set(PostgreSQL_PATH "/opt/homebrew/opt/postgresql@15")

    include_directories(/opt/homebrew/include ${LIBPQXX_PATH}/include ${PostgreSQL_PATH}/include)
    link_directories(/opt/homebrew/lib ${LIBPQXX_PATH}/lib ${PostgreSQL_PATH}/lib)

    set(LIBPQXX_LIBRARIES "${LIBPQXX_PATH}/lib/libpqxx-7.9.dylib")
    set(LIBPQXX_INCLUDE_DIRS "${LIBPQXX_PATH}/include")
    set(ENV{PKG_CONFIG_PATH} "/opt/homebrew/opt/libpqxx/lib/pkgconfig:/opt/homebrew/opt/postgresql@15/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endif ()

find_package(PostgreSQL REQUIRED)

if (NOT DEFINED LIBPQXX_LIBRARIES)
    message(FATAL_ERROR "libpqxx not found. Please install:\n  brew install libpqxx postgresql@15\n  brew link --force libpqxx\n  brew link --force postgresql@15")
endif ()

add_library(libpqxx::pqxx SHARED IMPORTED)
set_target_properties(libpqxx::pqxx PROPERTIES
        IMPORTED_LOCATION "${LIBPQXX_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBPQXX_INCLUDE_DIRS}"
)

# Testing Dependencies
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
find_package(GTest REQUIRED)

################################################################################
# 5. Main Library Target
################################################################################
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES}
        include/nuansa/handler/websocket_client.h
        include/nuansa/handler/websocket_server.h
        include/nuansa/services/auth/auth_service.h
        include/nuansa/services/user/iuser_service.h
        include/nuansa/services/user/user_service.h
        include/nuansa/plugin/iplugin.h
        include/nuansa/plugin/plugin_manager.h
        include/nuansa/services/user/iuser_service.h
        include/nuansa/services/user/user_service.h
        include/nuansa/handler/websocket_client.h
        include/nuansa/handler/websocket_handler.h
        include/nuansa/handler/websocket_server.h
        include/nuansa/handler/websocket_state_machine.h
        include/nuansa/handler/websocket_state_machine.h
        include/nuansa/utils/log/log.h)
target_include_directories(${PROJECT_NAME}_lib
        PUBLIC ${CMAKE_SOURCE_DIR}/include
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${Boost_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIR}
        GTest::GTest GTest::Main
)

# Precompiled Headers
target_precompile_headers(${PROJECT_NAME}_lib PRIVATE include/nuansa/pch/pch.h)
if (MSVC)
    target_precompile_headers(${PROJECT_NAME}_lib PUBLIC ${CMAKE_SOURCE_DIR}/include/nuansa/pch/pch.h)
else ()
    target_precompile_headers(${PROJECT_NAME}_lib PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/include/nuansa/pch/pch.h>")
endif ()

# Library Dependencies
target_link_libraries(${PROJECT_NAME}_lib PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::program_options
        Boost::log
        Boost::log_setup
        yaml-cpp::yaml-cpp
        libpqxx::pqxx
        PostgreSQL::PostgreSQL
)

# Platform-specific Configuration
if (UNIX)
    if (IS_MACOS)
        target_compile_definitions(${PROJECT_NAME}_lib PRIVATE USE_COMMON_CRYPTO)
    else ()
        target_link_libraries(${PROJECT_NAME}_lib PRIVATE crypt)
    endif ()
endif ()

################################################################################
# 6. Executables
################################################################################
# Main Executable
add_executable(${PROJECT_NAME} ${SOURCE_DIR}/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib Threads::Threads)

# Test Executables
add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES}
        tests/main.cpp
        tests/utils/db_test.h.cpp)
add_executable(user_service_test tests/unit/services/user/user_service_test.cpp)
add_executable(websocket_handler_test tests/unit/handlers/websocket_handler_test.cpp)

# Configure Test Executables
foreach (TEST_TARGET ${PROJECT_NAME}_tests user_service_test websocket_handler_test)
    set_target_properties(${TEST_TARGET}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
            RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/tests/Debug
            RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/tests/Release
    )
    target_link_libraries(${TEST_TARGET} PRIVATE
            GTest::gtest_main
            GTest::gmock_main
            ${PROJECT_NAME}_lib
    )
    target_include_directories(${TEST_TARGET} PRIVATE
            ${CMAKE_SOURCE_DIR}/tests
    )
endforeach ()

################################################################################
# 7. Testing Configuration
################################################################################
enable_testing()

# Register tests
add_test(NAME user_service_tests COMMAND user_service_test)
add_test(NAME websocket_handler_tests COMMAND websocket_handler_test)

# Test Runner Target
add_custom_target(run_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running all tests..."
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS user_service_test websocket_handler_test
        COMMENT "Running tests..."
)

################################################################################
# 8. Coverage Configuration
################################################################################
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if (ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # MacOS/Clang configuration
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")

        # Add custom target for generating coverage report
        add_custom_target(coverage
                # Create coverage directory
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/coverage

                # Run all test executables individually
                COMMAND ${CMAKE_COMMAND} -E echo "Running tests for coverage..."
                COMMAND ${CMAKE_BINARY_DIR}/bin/tests/${PROJECT_NAME}_tests
                COMMAND ${CMAKE_BINARY_DIR}/bin/tests/user_service_test
                COMMAND ${CMAKE_BINARY_DIR}/bin/tests/websocket_handler_test

                # Generate coverage report
                COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
                COMMAND llvm-profdata merge -sparse default.profraw -o ${CMAKE_BINARY_DIR}/bin/coverage/coverage.profdata
                COMMAND llvm-cov show
                ${CMAKE_BINARY_DIR}/bin/tests/${PROJECT_NAME}_tests
                ${CMAKE_BINARY_DIR}/bin/tests/user_service_test
                ${CMAKE_BINARY_DIR}/bin/tests/websocket_handler_test
                -instr-profile=${CMAKE_BINARY_DIR}/bin/coverage/coverage.profdata
                -format=html
                -output-dir=${CMAKE_BINARY_DIR}/bin/coverage
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PROJECT_NAME}_tests user_service_test websocket_handler_test
        )
    else ()
        # GCC configuration
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")

        # Find required tools
        find_program(GCOV gcov REQUIRED)
        find_program(GCOVR gcovr REQUIRED)

        # Add custom target for generating coverage report
        add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin/coverage
                COMMAND ${GCOVR} --root ${CMAKE_SOURCE_DIR}
                --html --html-details
                --output ${CMAKE_BINARY_DIR}/bin/coverage/index.html
                --object-directory ${CMAKE_BINARY_DIR}
                --exclude ".*tests/.*"
                --exclude ".*third_party/.*"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS run_tests
        )
    endif ()

    # Link with required libraries
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_libraries(${PROJECT_NAME}_lib PRIVATE -fprofile-instr-generate)
    else ()
        target_link_libraries(${PROJECT_NAME}_lib PRIVATE gcov)
    endif ()
endif ()

################################################################################
# 9. Utility Functions and Targets
################################################################################
# Config file copying function
function(copy_config_files TARGET_NAME)
    add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.yml
            $<TARGET_FILE_DIR:${TARGET_NAME}>/config.yml
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/.env
            $<TARGET_FILE_DIR:${TARGET_NAME}>/.env
            COMMENT "Copying configuration files for ${TARGET_NAME}"
    )
endfunction()

# Apply config copying to all targets
copy_config_files(${PROJECT_NAME})
foreach (TEST_TARGET ${PROJECT_NAME}_tests user_service_test websocket_handler_test)
    copy_config_files(${TEST_TARGET})
endforeach ()

# Custom targets
add_custom_target(run
        COMMAND ${CMAKE_COMMAND} -E echo "Running ${PROJECT_NAME}..."
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
)

add_custom_target(clean_build
        COMMAND ${CMAKE_COMMAND} -E echo "Cleaning build directory..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/coverage
        DEPENDS ${PROJECT_NAME}
)

if (ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
    target_compile_options(${PROJECT_NAME}_lib PRIVATE --coverage)
    target_link_options(${PROJECT_NAME}_lib PRIVATE --coverage)
endif ()

