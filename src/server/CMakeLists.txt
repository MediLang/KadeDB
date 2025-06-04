cmake_minimum_required(VERSION 3.14)

# Server source files
set(SERVER_SOURCES
    src/main.cpp
    # Add other server source files here
)

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
        $<BUILD_INTERFACE:kadedb_core>
        $<INSTALL_INTERFACE:KadeDB::Core>
        ${CMAKE_THREAD_LIBS_INIT}
        KadeDB  # Link against the main KadeDB library
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

# Installation
install(TARGETS kadedb
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

# Export targets for use from the build tree
export(TARGETS kadedb
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
)

# Create a KadeDBConfig.cmake file for the build tree
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
    @ONLY
)

# Install the export set for use with the install-tree
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/KadeDB
)

# Generate the config file for the build tree
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
    INSTALL_DESTINATION lib/cmake/KadeDB
)

# Generate the config version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config file
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
    DESTINATION lib/cmake/KadeDB
)

# Enable testing if requested
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

# Add uninstall target
if(NOT TARGET uninstall)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()

# Install headers
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
