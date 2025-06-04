#!/bin/bash
# Verify KadeDB dependency management system

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to print section header
print_section() {
    echo -e "\n${GREEN}=== $1 ===${NC}"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
print_section "Checking for required tools"
REQUIRED_TOOLS=("cmake" "git" "make" "g++" "pkg-config")
MISSING_TOOLS=0

for tool in "${REQUIRED_TOOLS[@]}"; do
    if command_exists "$tool"; then
        echo -e "${GREEN}✓${NC} $tool is installed"
    else
        echo -e "${RED}✗${NC} $tool is not installed"
        MISSING_TOOLS=1
    fi
done

if [ $MISSING_TOOLS -ne 0 ]; then
    echo -e "\n${RED}Error: Missing required tools. Please install them and try again.${NC}"
    exit 1
fi

# Create build directory
BUILD_DIR="${BUILD_DIR:-build}"
print_section "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with system dependencies
print_section "Configuring with system dependencies"
if cmake .. -DUSE_SYSTEM_DEPS=ON -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=ON; then
    echo -e "${GREEN}✓ Successfully configured with system dependencies${NC}"
else
    echo -e "${RED}✗ Failed to configure with system dependencies${NC}"
    echo -e "Trying with bundled dependencies..."
    
    # Clean build directory
    cd ..
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with bundled dependencies
    print_section "Configuring with bundled dependencies"
    if ! cmake .. -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=ON; then
        echo -e "${RED}✗ Failed to configure with bundled dependencies${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ Successfully configured with bundled dependencies${NC}"
fi

# Build the project
print_section "Building the project"
if make -j$(nproc); then
    echo -e "${GREEN}✓ Successfully built the project${NC}
"
    echo -e "${GREEN}Dependency management system verification completed successfully!${NC}"
    echo -e "\nYou can now run the following commands to test the installation:"
    echo -e "  cd $BUILD_DIR"
    echo -e "  ctest --output-on-failure"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi
