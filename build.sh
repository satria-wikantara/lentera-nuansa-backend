#!/bin/bash

# Define the build type (Debug or Release)
BUILD_TYPE=${1:-Debug}

# Function to clean the build and bin directories
clean() {
    echo "Cleaning build and bin directories..."
    rm -rf build
    rm -rf bin
    echo "Clean completed."
}

# Function to run the executable
run() {
    local build_type=${1:-Debug}
    local executable="./build/bin/${build_type}/nuansa"
    local config_path=${2:-"config.yml"}
    
    if [ ! -f "$executable" ]; then
        echo "Error: Executable not found at $executable"
        echo "Please build the project first."
        exit 1
    fi
    
    echo "Running nuansa..."
    $executable run --config "$config_path"
}

# Check command argument
case "$1" in
    "clean")
        clean
        exit 0
        ;;
    "rebuild")
        clean
        BUILD_TYPE=${2:-Debug}
        ;;
    "run")
        run ${2:-Debug} ${3:-"config.yml"}
        exit 0
        ;;
esac

# Create a build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Navigate to the build directory
cd build

# Run CMake to configure the project
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# Build the project
cmake --build .

# Navigate back to the root directory
cd ..

# Print a message indicating the build is complete
echo "Build completed. Executable is located in bin/$BUILD_TYPE/"

# If additional argument is "run", execute the program
if [ "$2" == "run" ]; then
    run $BUILD_TYPE "config.yml"
fi