# Define the storage library
add_library(kadedb_storage
    src/file_manager.cpp
    src/page/page.cpp
    src/page/page_manager.cpp
)

# Set the output name to avoid conflicts
set_target_properties(kadedb_storage PROPERTIES OUTPUT_NAME kadedb_storage)

# Set target properties
set_target_properties(kadedb_storage PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    EXPORT_NAME kadedb_storage
)

# Set include directories
target_include_directories(kadedb_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(kadedb_storage
    PRIVATE 
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
        kadedb_common
        crc32c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set compile definitions
target_compile_definitions(kadedb_storage
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG=1>
)

# Set compile options
target_compile_options(kadedb_storage
    PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
)

# Add tests if available
if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    add_subdirectory(test)
endif()

# Set include directories for kadedb_storage
target_include_directories(kadedb_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Create a proper include directory structure for crc32c
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/crc32c)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/crc32c/crc32c.h
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/include/kadedb/storage/crc32c.h
        ${CMAKE_CURRENT_BINARY_DIR}/crc32c/crc32c.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/kadedb/storage/crc32c.h
    COMMENT "Creating crc32c include symlink"
)

add_custom_target(crc32c_headers ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/crc32c/crc32c.h
)

add_dependencies(kadedb_storage crc32c_headers)

# Add the binary directory to include paths for the build tree
target_include_directories(kadedb_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Set the include directory for the crc32c header
target_include_directories(kadedb_storage
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Note: Installation is handled by the main CMakeLists.txt
