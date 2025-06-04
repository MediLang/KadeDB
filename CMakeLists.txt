cmake_minimum_required(VERSION 3.14)

# Function to filter sensitive environment variables
function(filter_sensitive_variables)
    # List of sensitive variable patterns to filter
    set(SENSITIVE_VARS
        "API[_\-]KEY"
        "SECRET"
        "TOKEN"
        "PASSWORD"
        "AUTH"
        "CREDENTIAL"
        "ANTHROPIC"
        "PERPLEXITY"
        "OPENAI"
    )
    
    # Get all environment variables
    execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "environment"
                    OUTPUT_VARIABLE ENV_VARS)
    
    # Filter out sensitive variables
    foreach(VAR ${SENSITIVE_VARS})
        string(REGEX REPLACE "${VAR}=[^\n]*\n?" "" ENV_VARS "${ENV_VARS}")
    endforeach()
    
    # Set filtered environment
    set(ENV{ANTHROPIC_API_KEY} "")
    set(ENV{PERPLEXITY_API_KEY} "")
    set(ENV{OPENAI_API_KEY} "")
endfunction()

# Call the function to filter sensitive variables
filter_sensitive_variables()

project(KadeDB VERSION 0.1.0 LANGUAGES CXX C ASM)

# Set project policies
cmake_policy(SET CMP0079 NEW) # Enable compiler id for AppleClang

# Set C++ standard to C++20 for std::span and other C++20 features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Only treat warnings as errors in Debug builds
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-Werror)
    endif()
    # Disable specific warnings that might come from external dependencies
    add_compile_options(-Wno-pedantic -Wno-error=pedantic)
    add_compile_options(-Wno-error=extra)
    add_compile_options(-Wno-array-bounds)  # Disable array bounds warnings
endif()

# Include standard installation directories
include(GNUInstallDirs)

# Project options
option(BUILD_KADEDB_SERVER "Build KadeDB Server" ON)
option(BUILD_KADEDB_LITE "Build KadeDB-Lite" ON)
option(BUILD_TESTS "Build tests" OFF)  # Disabled by default
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
option(ENABLE_IPO "Enable Interprocedural Optimization" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(USE_SYSTEM_DEPS "Use system-installed dependencies when available" OFF)
option(USE_SYSTEM_ROCKSDB "Use system-installed RocksDB" ${USE_SYSTEM_DEPS})
option(USE_SYSTEM_ANTLR "Use system-installed ANTLR" ${USE_SYSTEM_DEPS})

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set RPATH for installed binaries
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Compiler flags
include(CheckCXXCompilerFlag)

# Common compiler flags
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Platform-specific settings
if(MSVC)
    add_compile_options(/W4 /WX /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
    if(MSVC_VERSION GREATER_EQUAL 1910)
        add_compile_options(/permissive-)
    endif()
else()
    # GCC/Clang flags
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-Wno-unused-parameter -Wno-missing-field-initializers)
    
    # Linker flags
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
    
    # Position Independent Code
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
    # Link Time Optimization
    if(ENABLE_LTO)
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported)
        if(ipo_supported)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        else()
            message(WARNING "LTO is not supported by the compiler")
        endif()
    endif()
endif()

# Sanitizers
if(ENABLE_ASAN OR ENABLE_UBSAN)
    if(MSVC)
        message(WARNING "Sanitizers are not supported on MSVC")
    else()
        set(SANITIZER_FLAGS "")
        if(ENABLE_ASAN)
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        endif()
        if(ENABLE_UBSAN)
            set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined -fno-sanitize-recover=all")
        endif()
        
        add_compile_options(${SANITIZER_FLAGS})
        add_link_options(${SANITIZER_FLAGS})
    endif()
endif()

# Code coverage
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)
    else()
        message(WARNING "Code coverage is only supported for GCC and Clang")
    endif()
endif()

# Include dependency management
include(cmake/dependencies.cmake)

# Set project version components
set(PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} CACHE STRING "Major version")
set(PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR} CACHE STRING "Minor version")
set(PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH} CACHE STRING "Patch version")

# Configure config.h
configure_file(
    ${PROJECT_SOURCE_DIR}/include/kadedb/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/kadedb/config.h
    @ONLY
)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${PROJECT_SOURCE_DIR}/include
)

# Add core library
# Set binary output directories for server and lite
set(SERVER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/server)
set(LITE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/lite)

# Set binary output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add common interface library
add_library(kadedb_common INTERFACE)
target_include_directories(kadedb_common
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Find or provide CRC32C implementation
find_package(Crc32c QUIET)

if(TARGET Crc32c::crc32c)
    # Use system CRC32C if available
    message(STATUS "Using system CRC32C library")
    add_library(crc32c ALIAS Crc32c::crc32c)
else()
    # Fallback: Create a minimal CRC32C implementation
    message(STATUS "Using bundled CRC32C implementation")
    
    # Create the source file if it doesn't exist
    set(CRC32C_SRC ${CMAKE_CURRENT_BINARY_DIR}/crc32c.cpp)
    if(NOT EXISTS ${CRC32C_SRC})
        file(WRITE ${CRC32C_SRC} "#include <cstdint>\n"
            "#include <cstddef>\n\n"
            "uint32_t crc32c_sw(const uint8_t* data, std::size_t length) {\n"
            "    uint32_t crc = 0xFFFFFFFF;\n"
            "    for (std::size_t i = 0; i < length; ++i) {\n"
            "        crc ^= data[i];\n"
            "        for (int j = 0; j < 8; ++j) {\n"
            "            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));\n"
            "        }\n"
            "    }\n"
            "    return ~crc;\n"
            "}"
        )
    endif()
    
    # Create the library
    add_library(crc32c STATIC
        ${CRC32C_SRC}
    )
    
    target_include_directories(crc32c
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core/storage/include>
    )
    
    # Add include directory for the generated file
    target_include_directories(crc32c
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # Add compile definitions
    target_compile_definitions(crc32c
        PRIVATE
            -DCRC32C_INLINE_DEFINITION
    )
    
    # Hide symbols by default
    set_target_properties(crc32c PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

# Add core library
add_subdirectory(src/core)

# Add server build if enabled
if(BUILD_KADEDB_SERVER)
    add_subdirectory(src/server)
endif()

# Add lite build if enabled
if(BUILD_KADEDB_LITE)
    enable_language(C)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    add_subdirectory(src/lite)
endif()

# Add tests if enabled
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

# Install headers
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Create a custom target that depends on all components
add_custom_target(kadedb_all
    DEPENDS 
        kadedb_common
        crc32c
        kadedb_storage
        kadedb_core
        $<TARGET_NAME_IF_EXISTS:kadedb>
)

# Install all targets with proper export
install(TARGETS 
        kadedb_common
        crc32c
        kadedb_storage
        kadedb_core
    EXPORT KadeDBTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install server executable if built
if(TARGET kadedb)
    install(TARGETS kadedb
        EXPORT KadeDBTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Create a single export file for all targets
set(EXPORT_TARGETS kadedb_common crc32c kadedb_storage kadedb_core)
if(TARGET kadedb)
    list(APPEND EXPORT_TARGETS kadedb)
endif()

# Export for build tree
export(TARGETS ${EXPORT_TARGETS}
    NAMESPACE KadeDB::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/KadeDBTargets.cmake"
)

# Install export set
install(EXPORT KadeDBTargets
    NAMESPACE KadeDB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KadeDB
    FILE KadeDBTargets.cmake
)

# Ensure proper build order
add_dependencies(kadedb_storage kadedb_common crc32c)
add_dependencies(kadedb_core kadedb_storage)
if(TARGET kadedb)
    add_dependencies(kadedb kadedb_core)
endif()

# Generate and install config files
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/KadeDBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
    INSTALL_DESTINATION lib/cmake/KadeDB
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
    DESTINATION lib/cmake/KadeDB
)

# Uninstall target
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

# Install rules
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(EXPORT ${PROJECT_NAME}Targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
)

configure_file(cmake/KadeDBConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    @ONLY
)

set(ConfigPackageLocation lib/cmake/${PROJECT_NAME})
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${ConfigPackageLocation}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${ConfigPackageLocation}
    COMPONENT Devel
)

# Only add uninstall target if not already defined
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
