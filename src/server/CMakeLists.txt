# Server source files
set(SERVER_SOURCES
    src/main.cpp
    # Add other server source files here
)

# Set the output name to avoid conflicts
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# Add server executable
add_executable(kadedb
    ${SERVER_SOURCES}
)

# Set target properties
set_target_properties(kadedb PROPERTIES
    OUTPUT_NAME kadedb
    VERSION ${PROJECT_VERSION}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Set compile features
target_compile_features(kadedb PRIVATE cxx_std_17)

# Include directories
target_include_directories(kadedb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/include
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(kadedb
    PRIVATE
        kadedb_core  # Link against the core library
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        ${ROCKSDB_LIBRARIES}
        ${ANTLR4_RUNTIME_LIBRARIES}
)

# Add compile definitions
target_compile_definitions(kadedb
    PRIVATE
        $<$<CONFIG:Debug>:KADEDB_DEBUG>
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:RelWithDebInfo>:NDEBUG>
        $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# Note: Installation is handled by the main CMakeLists.txt

# Testing and benchmarks are controlled by the main CMakeLists.txt

# Uninstall target is handled by the main CMakeLists.txt
