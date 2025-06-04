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

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project options
option(BUILD_KADEDB_SERVER "Build KadeDB Server" ON)
option(BUILD_KADEDB_LITE "Build KadeDB-Lite" ON)
option(BUILD_TESTS "Build tests" OFF)
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

# Add subdirectories
add_subdirectory(src/core)

# Add server if enabled
if(BUILD_KADEDB_SERVER)
    add_subdirectory(src/server)
endif()

# Add KadeDB-Lite if enabled
if(BUILD_KADEDB_LITE)
    add_subdirectory(src/lite)
endif()

# Add tests if enabled
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Generate and install package configuration
include(CMakePackageConfigHelpers)
set(CONFIG_INSTALL_DIR "lib/cmake/KadeDB-${PROJECT_VERSION}")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Create the KadeDBConfig.cmake file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBConfig.cmake.in
    ${GENERATED_DIR}/KadeDBConfig.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
)

# Create the KadeDBConfigVersion.cmake file
write_basic_package_version_file(
    ${GENERATED_DIR}/KadeDBConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the configuration files
install(
    FILES
        ${GENERATED_DIR}/KadeDBConfig.cmake
        ${GENERATED_DIR}/KadeDBConfigVersion.cmake
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Install the export set
install(
    EXPORT KadeDBTargets
    FILE KadeDBTargets.cmake
    NAMESPACE KadeDB::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Install headers
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
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

# Add subdirectories after the main library is defined
if(BUILD_KADEDB_SERVER)
    add_subdirectory(src/server)
endif()

if(BUILD_KADEDB_LITE)
    add_subdirectory(src/lite)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Install rules for the main library
install(TARGETS KadeDB
    EXPORT KadeDBTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/kadedb
)

# Install the export set
install(EXPORT KadeDBTargets
    FILE KadeDBTargets.cmake
    NAMESPACE KadeDB::
    DESTINATION lib/cmake/KadeDB
)

# Create and install package config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBConfig.cmake.in
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

# Add uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
