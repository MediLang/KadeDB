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

# Define source files for the lite executable
set(LITE_SOURCES
    src/main.c
)

# Add KadeDB-Lite executable
add_executable(kadedb-lite_exe ${LITE_SOURCES})

# Set target properties
set_target_properties(kadedb-lite_exe PROPERTIES
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "kadedb-lite"
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)

# Include directories
target_include_directories(kadedb-lite_exe
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(kadedb-lite_exe
    PRIVATE
        Threads::Threads
        kadedb-lite
        # Add other dependencies as needed
)

# Add compile definitions
target_compile_definitions(kadedb-lite_exe
    PRIVATE
        $<$<CONFIG:Debug>:KADEDB_DEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# Installation
install(TARGETS kadedb-lite_exe
    EXPORT ${PROJECT_NAME}LiteTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(TARGETS kadedb-lite
    EXPORT ${PROJECT_NAME}LiteTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Export targets for use from the build tree
export(TARGETS kadedb-lite_exe kadedb-lite
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}LiteTargets.cmake
    NAMESPACE ${PROJECT_NAME}::
)

# Create a KadeDBLiteConfig.cmake file for the build tree
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBLiteConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBLiteConfig.cmake
    @ONLY
)

# Install the export set for use with the install-tree
install(EXPORT ${PROJECT_NAME}LiteTargets
    FILE ${PROJECT_NAME}LiteTargets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/KadeDBLite
)

# Install headers
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
