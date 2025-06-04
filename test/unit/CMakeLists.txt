cmake_minimum_required(VERSION 3.14)

# Include GoogleTest
include(GoogleTest)

# Add storage tests
add_subdirectory(storage)

# Add any test-specific includes or definitions here

# Set common test properties
set(TEST_LIBRARIES
    kadedb_storage
)

# Add any common compile definitions for tests
add_compile_definitions(
    UNIT_TESTS
)
