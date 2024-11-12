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

# Check for the clean or rebuild argument
if [ "$1" == "clean" ]; then
    clean
    exit 0
elif [ "$1" == "rebuild" ]; then
    clean
    echo "Rebuilding project..."
    BUILD_TYPE=${2:-Debug}  # Allow specifying build type for rebuild
fi

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