cmake_minimum_required(VERSION 3.14)

# Define the KadeDB-Lite library
add_library(kadedb-lite
    src/main.c
)

target_include_directories(kadedb-lite
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(kadedb-lite
    PRIVATE
        Threads::Threads
)

# Set compile definitions for lite version
target_compile_definitions(kadedb-lite
    PRIVATE
        KADEDB_LITE=1
)

# Install rules
install(TARGETS kadedb-lite
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
