cmake_minimum_required(VERSION 3.14)
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
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
option(ENABLE_IPO "Enable Interprocedural Optimization" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(USE_SYSTEM_ROCKSDB "Use system-installed RocksDB" OFF)
option(USE_SYSTEM_ANTLR "Use system-installed ANTLR" OFF)

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

# Find dependencies
find_package(Threads REQUIRED)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Find RocksDB
if(USE_SYSTEM_ROCKSDB)
    find_package(RocksDB REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.0.0
    )
    set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
    set(WITH_JEMALLOC OFF CACHE BOOL "" FORCE)
    set(WITH_LZ4 OFF CACHE BOOL "" FORCE)
    set(WITH_SNAPPY OFF CACHE BOOL "" FORCE)
    set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
    set(WITH_ZSTD OFF CACHE BOOL "" FORCE)
    set(WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
    set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(rocksdb)
    set(ROCKSDB_LIBRARIES rocksdb)
endif()

# Find ANTLR
if(USE_SYSTEM_ANTLR)
    find_package(Antlr4 REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        antlr4
        GIT_REPOSITORY https://github.com/antlr/antlr4.git
        GIT_TAG 4.9.3
    )
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(antlr4)
    set(ANTLR4_INCLUDE_DIRS ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src)
    set(ANTLR4_RUNTIME_LIBRARIES antlr4_shared)
endif()

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
    ${ANTLR4_INCLUDE_DIRS}
)

# Add subdirectories
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
