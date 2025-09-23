cmake_minimum_required(VERSION 3.16)

project(KadeDB
  VERSION 0.1.0
  DESCRIPTION "KadeDB - A multi-modal database for healthcare and beyond"
  LANGUAGES C CXX
)

# Build options
option(KADEDB_BUILD_SHARED "Build shared libraries" ON)
option(KADEDB_LITE_WITH_ROCKSDB "Build KadeDB-Lite with RocksDB dependency" OFF)
option(KADEDB_ENABLE_ASAN "Enable AddressSanitizer (non-MSVC)" OFF)
option(KADEDB_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (non-MSVC)" OFF)
option(KADEDB_ENABLE_COVERAGE "Enable coverage flags (gcc/clang, non-MSVC)" OFF)

# Standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Global properties
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(GNUInstallDirs)
include(CTest) # defines BUILD_TESTING and enables testing by default

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sanitizers / Coverage (non-MSVC)
if(NOT MSVC)
  if(KADEDB_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif()
  if(KADEDB_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
  if(KADEDB_ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
    # Coverage report target (requires gcovr)
    find_program(GCOVR_EXECUTABLE gcovr)
    if(GCOVR_EXECUTABLE)
      set(KADEDB_COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
      file(MAKE_DIRECTORY ${KADEDB_COVERAGE_DIR})
      add_custom_target(coverage
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${GCOVR_EXECUTABLE}
                -r ${CMAKE_SOURCE_DIR}
                --exclude '.*build/.*'
                --exclude '.*bindings/.*'
                --exclude '.*install/.*'
                --exclude '.*docs/.*'
                --exclude '.*cmake/.*'
                --exclude '.*lite/.*' # exclude lite in core coverage; it has its own tests
                --xml=${KADEDB_COVERAGE_DIR}/coverage.xml
                --html-details=${KADEDB_COVERAGE_DIR}/index.html
                --print-summary
                --fail-under-line=55
                --fail-under-branch=30
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report with gcovr"
        VERBATIM)
    else()
      message(WARNING "gcovr not found; 'coverage' target will not be available. Install gcovr to generate coverage reports.")
    endif()
  endif()
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Version header configuration
set(KADEDB_VERSION_HEADER_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/kadedb/version.h.in)
set(KADEDB_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${KADEDB_GENERATED_INCLUDE_DIR}/kadedb)
configure_file(${KADEDB_VERSION_HEADER_INPUT}
               ${KADEDB_GENERATED_INCLUDE_DIR}/kadedb/version.h @ONLY)

# Subdirectories
add_subdirectory(cpp)
add_subdirectory(bindings/c)
add_subdirectory(lite)

# Tests
if(BUILD_TESTING)
  add_subdirectory(cpp/test)
  add_subdirectory(lite/test)
endif()

include(CMakePackageConfigHelpers)

# Generate package version file
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Configure package config file
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KadeDBConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KadeDB
)

# Install headers (public) and generated version header
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bindings/c/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lite/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")
install(FILES ${KADEDB_GENERATED_INCLUDE_DIR}/kadedb/version.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kadedb)

# Export and install targets
install(EXPORT KadeDBTargets
        NAMESPACE KadeDB::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KadeDB)

# Install package config files
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/KadeDBConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/KadeDB
)
