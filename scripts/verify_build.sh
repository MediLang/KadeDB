#!/bin/bash

# Exit on error
set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run a command with logging
run_command() {
    echo -e "${YELLOW}Running: $@${NC}"
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo -e "${RED}Error: Command failed with status $status${NC}" >&2
        exit $status
    fi
    echo -e "${GREEN}Success${NC}\n"
    return $status
}

# Check if we're in the correct directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Clean previous builds
if [ -d "build" ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

# Test different build configurations
for BUILD_TYPE in Debug Release; do
    echo -e "\n${YELLOW}=== Testing $BUILD_TYPE build ===${NC}"
    
    # Configure
    run_command cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=ON
    
    # Build
    run_command cmake --build . -- -j$(nproc)
    
    # Run tests if available
    if [ -f "CTestTestfile.cmake" ]; then
        run_command ctest --output-on-failure
    else
        echo -e "${YELLOW}No tests found${NC}"
    fi
    
    # Clean up
    cd ..
    rm -rf build
    mkdir -p build
    cd build
done

# Test with sanitizers if not on Windows
if [[ "$OSTYPE" != "msys" && "$OSTYPE" != "win32" ]]; then
    echo -e "\n${YELLOW}=== Testing with Address Sanitizer ===${NC}"
    run_command cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DBUILD_TESTS=ON
    run_command cmake --build . -- -j$(nproc)
    
    if [ -f "CTestTestfile.cmake" ]; then
        run_command ctest --output-on-failure
    fi
    
    # Clean up
    cd ..
    rm -rf build
    mkdir -p build
    cd build
    
    echo -e "\n${YELLOW}=== Testing with Undefined Behavior Sanitizer ===${NC}"
    run_command cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON -DBUILD_TESTS=ON
    run_command cmake --build . -- -j$(nproc)
    
    if [ -f "CTestTestfile.cmake" ]; then
        run_command ctest --output-on-failure
    fi
fi

# Test installation
echo -e "\n${YELLOW}=== Testing installation ===${NC}"
INSTALL_PREFIX="${PWD}/install"
run_command cmake .. -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
run_command cmake --build . --target install -- -j$(nproc)

# Verify installed files
if [ -f "${INSTALL_PREFIX}/bin/kadedb" ] || [ -f "${INSTALL_PREFIX}/bin/kadedb.exe" ]; then
    echo -e "${GREEN}Installation verified${NC}
"
else
    echo -e "${YELLOW}Warning: kadedb binary not found in installation directory${NC}"
fi

# Clean up
cd ..
rm -rf build

echo -e "\n${GREEN}All tests completed successfully!${NC}"
