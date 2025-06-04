# Core Database Library

# Define the core library target
add_library(kadedb_core
    kadedb.cpp
    # Add other core source files here
)

# Set target properties
set_target_properties(kadedb_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Set include directories
target_include_directories(kadedb_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${ROCKSDB_INCLUDE_DIRS}
        ${ANTLR4_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(kadedb_core
    PUBLIC
        RocksDB::RocksDB
        OpenSSL::SSL
        OpenSSL::Crypto
        ${ANTLR4_RUNTIME_LIBRARIES}
)

# Set compile definitions
target_compile_definitions(kadedb_core
    PRIVATE
        $<$<CONFIG:Debug>:KADEDB_DEBUG>
        $<$<CONFIG:Release>:KADEDB_RELEASE>
        $<$<CONFIG:RelWithDebInfo>:KADEDB_RELEASE>
        $<$<CONFIG:MinSizeRel>:KADEDB_RELEASE>
)

# Generate export header
include(GenerateExportHeader)
generate_export_header(kadedb_core
    BASE_NAME KADEDB_CORE
    EXPORT_MACRO_NAME KADEDB_CORE_EXPORT
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/kadedb/core/export.h
    STATIC_DEFINE KADEDB_CORE_STATIC_DEFINE
)

# Create a config.h file
configure_file(
    ${PROJECT_SOURCE_DIR}/include/kadedb/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/kadedb/config.h
    @ONLY
)

# Include generated headers
target_include_directories(kadedb_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Installation
install(TARGETS kadedb_core
    EXPORT KadeDBTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/kadedb
)

# Install headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/kadedb/
    DESTINATION include/kadedb
    FILES_MATCHING PATTERN "*.h"
)
