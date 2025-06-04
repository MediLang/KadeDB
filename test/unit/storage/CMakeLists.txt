# Add test executable
add_executable(storage_tests
    file_manager_test.cpp
    page_manager_test.cpp
)

# Link with GTest and our storage library
target_link_libraries(storage_tests
    PRIVATE
        GTest::GTest
        GTest::Main
        kadedb_storage
        ${TEST_LIBRARIES}
)

# Add include directories
target_include_directories(storage_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME storage_tests
    COMMAND storage_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Enable test discovery for IDEs
gtest_discover_tests(storage_tests
    PROPERTIES
        TIMEOUT 30
)
