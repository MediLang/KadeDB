cmake_minimum_required(VERSION 3.14)

# Enable testing
enable_testing()

# Find GoogleTest
find_package(GTest REQUIRED)
include(GoogleTest)

# Add unit tests subdirectory
add_subdirectory(unit)

# Add integration tests subdirectory
add_subdirectory(integration)

# Add benchmark tests subdirectory
add_subdirectory(benchmark)

# Function to add a test
function(add_kadedb_test test_name)
    add_executable(${test_name} ${ARGN})
    target_link_libraries(${test_name}
        PRIVATE
            GTest::GTest
            GTest::Main
            kadedb
    )
    gtest_discover_tests(${test_name})
endfunction()

# Function to add a benchmark test
function(add_kadedb_benchmark benchmark_name)
    add_executable(${benchmark_name} ${ARGN})
    target_link_libraries(${benchmark_name}
        PRIVATE
            benchmark::benchmark
            kadedb
    )
endfunction()
