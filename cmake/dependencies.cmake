# KadeDB Dependency Management
# ==========================
# This file handles all external dependencies for the KadeDB project.
# It provides options to use system-installed libraries or download and build them.

# Add the modules directory to the module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Default to bundled dependencies if not specified
option(USE_SYSTEM_DEPS "Use system-installed dependencies when available" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Set default dependency versions
set(KADEDB_ROCKSDB_VERSION "8.0.0" CACHE STRING "RocksDB version to use")
set(KADEDB_ANTLR_VERSION "4.9.3" CACHE STRING "ANTLR4 version to use")
set(KADEDB_OPENSSL_VERSION "1.1.1" CACHE STRING "OpenSSL version to use")

# Find or download dependencies
include(FetchContent)
include(ExternalProject)

# Function to safely add subdirectory with error checking
function(kadedb_add_subdirectory dir)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
    add_subdirectory(${dir} ${ARGN})
  else()
    message(WARNING "Directory ${dir} does not contain CMakeLists.txt")
  endif()
endfunction()

# Find or download OpenSSL
if(USE_SYSTEM_DEPS)
  find_package(OpenSSL ${KADEDB_OPENSSL_VERSION} REQUIRED)
else()
  # Use the system OpenSSL for now, but could use FetchContent or ExternalProject
  find_package(OpenSSL REQUIRED)
  if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found and system dependencies are required")
  endif()
endif()

# Find or download RocksDB
if(USE_SYSTEM_DEPS)
  # First try using pkg-config
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_ROCKSDB QUIET rocksdb)
    if(PC_ROCKSDB_FOUND)
      message(STATUS "Found RocksDB via pkg-config: ${PC_ROCKSDB_LIBRARIES}")
      set(ROCKSDB_LIBRARIES ${PC_ROCKSDB_LINK_LIBRARIES})
      set(ROCKSDB_INCLUDE_DIRS ${PC_ROCKSDB_INCLUDE_DIRS})
      set(ROCKSDB_VERSION ${PC_ROCKSDB_VERSION})
      set(USE_SYSTEM_ROCKSDB ON)
      
      # Create imported target if it doesn't exist
      if(NOT TARGET RocksDB::RocksDB)
        add_library(RocksDB::RocksDB INTERFACE IMPORTED)
        set_target_properties(RocksDB::RocksDB PROPERTIES
          INTERFACE_LINK_LIBRARIES "${ROCKSDB_LIBRARIES}"
          INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIRS}"
        )
      endif()
    endif()
  endif()
  
  # If pkg-config failed, try find_package
  if(NOT ROCKSDB_LIBRARIES)
    find_package(RocksDB ${KADEDB_ROCKSDB_VERSION} QUIET)
    if(RocksDB_FOUND)
      message(STATUS "Found RocksDB: ${RocksDB_LIBRARIES}")
      set(ROCKSDB_LIBRARIES ${RocksDB_LIBRARIES})
      set(ROCKSDB_INCLUDE_DIRS ${RocksDB_INCLUDE_DIRS})
      set(ROCKSDB_VERSION ${RocksDB_VERSION})
      set(USE_SYSTEM_ROCKSDB ON)
    else()
      message(STATUS "System RocksDB not found, using bundled version")
      set(USE_SYSTEM_ROCKSDB OFF)
    endif()
  endif()
else()
  set(USE_SYSTEM_ROCKSDB OFF)
  message(STATUS "Using bundled RocksDB (system libraries disabled)")
endif()

if(NOT USE_SYSTEM_ROCKSDB)
  message(STATUS "Using bundled RocksDB ${KADEDB_ROCKSDB_VERSION}")
  
  # Set RocksDB build options
  set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
  set(WITH_JEMALLOC OFF CACHE BOOL "" FORCE)
  set(WITH_LZ4 OFF CACHE BOOL "" FORCE)
  set(WITH_SNAPPY OFF CACHE BOOL "" FORCE)
  set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
  set(WITH_ZSTD OFF CACHE BOOL "" FORCE)
  set(WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
  set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
  set(FAIL_ON_WARNINGS OFF CACHE BOOL "" FORCE)
  set(WITH_BZ2 OFF CACHE BOOL "" FORCE)
  set(WITH_LIBURING OFF CACHE BOOL "" FORCE)
  set(WITH_NUMA OFF CACHE BOOL "" FORCE)
  set(WITH_TBB OFF CACHE BOOL "" FORCE)
  set(WITH_XPRESS OFF CACHE BOOL "" FORCE)
  set(WITH_JNI OFF CACHE BOOL "" FORCE)
  set(WITH_BENCHMARK OFF CACHE BOOL "" FORCE)
  set(WITH_CORE_TOOLS OFF CACHE BOOL "" FORCE)
  set(WITH_FOLLY_DISTRIBUTED_MUTEX OFF CACHE BOOL "" FORCE)
  
  # Download and build RocksDB
  FetchContent_Declare(
    rocksdb
    GIT_REPOSITORY https://github.com/facebook/rocksdb.git
    GIT_TAG v${KADEDB_ROCKSDB_VERSION}
    GIT_SHALLOW TRUE
  )
  
  # Only populate if not already populated
  FetchContent_GetProperties(rocksdb)
  if(NOT rocksdb_POPULATED)
    message(STATUS "Downloading and building RocksDB ${KADEDB_ROCKSDB_VERSION}...")
    FetchContent_Populate(rocksdb)
    
    # Create a CMakeLists.txt file if it doesn't exist
    if(NOT EXISTS "${rocksdb_SOURCE_DIR}/CMakeLists.txt")
      file(WRITE "${rocksdb_SOURCE_DIR}/CMakeLists.txt" 
        "cmake_minimum_required(VERSION 3.10)\n"
        "project(rocksdb VERSION ${KADEDB_ROCKSDB_VERSION} LANGUAGES C CXX ASM)\n\n"
        "option(WITH_TESTS \"Build tests\" OFF)\n"
        "option(WITH_TOOLS \"Build tools\" OFF)\n"
        "option(WITH_BENCHMARK_TOOLS \"Build benchmark tools\" OFF)\n"
        "option(FAIL_ON_WARNINGS \"Fail on compiler warnings\" OFF)\n"
        "add_subdirectory(${rocksdb_SOURCE_DIR})\n"
      )
    endif()
    
    # Don't build tests or tools
    add_subdirectory(${rocksdb_SOURCE_DIR} ${rocksdb_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  
  # Set the library names based on build type
  if(WIN32)
    set(ROCKSDB_LIBRARIES ${rocksdb_BINARY_DIR}/rocksdb.lib)
  else()
    set(ROCKSDB_LIBRARIES ${rocksdb_BINARY_DIR}/librocksdb.a)
  endif()
  
  set(ROCKSDB_INCLUDE_DIRS ${rocksdb_SOURCE_DIR}/include)
  
  # Create an imported target for RocksDB
  if(NOT TARGET RocksDB::RocksDB)
    add_library(RocksDB::RocksDB STATIC IMPORTED)
    set_target_properties(RocksDB::RocksDB PROPERTIES
      IMPORTED_LOCATION ${ROCKSDB_LIBRARIES}
      INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIRS}"
    )
    
    # Add system libraries that RocksDB depends on
    if(UNIX)
      find_package(Threads REQUIRED)
      target_link_libraries(RocksDB::RocksDB INTERFACE 
        ${CMAKE_THREAD_LIBS_INIT}
        ${CMAKE_DL_LIBS}
      )
      set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "Threads::Threads"
      )
    endif()
    if(CMAKE_DL_LIBS)
      set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS}
      )
    endif()
  endif()
endif()

# Find or download ANTLR4
if(USE_SYSTEM_DEPS)
  find_package(ANTLR4 ${KADEDB_ANTLR_VERSION} QUIET COMPONENTS runtime)
  if(NOT ANTLR4_FOUND)
    message(STATUS "System ANTLR4 not found, using bundled version")
    set(USE_SYSTEM_ANTLR OFF)
  else()
    set(USE_SYSTEM_ANTLR ON)
    message(STATUS "Found ANTLR4: ${ANTLR4_LIBRARIES}")
  endif()
else()
  set(USE_SYSTEM_ANTLR OFF)
endif()

if(NOT USE_SYSTEM_ANTLR)
  message(STATUS "Using bundled ANTLR4 ${KADEDB_ANTLR_VERSION}")
  
  # Set ANTLR4 build options
  set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
  set(ANTLR4_INSTALL OFF CACHE BOOL "" FORCE)
  
  # Download and build ANTLR4
  FetchContent_Declare(
    antlr4
    GIT_REPOSITORY https://github.com/antlr/antlr4.git
    GIT_TAG ${KADEDB_ANTLR_VERSION}
    GIT_SHALLOW TRUE
  )
  
  # Only populate if not already populated
  FetchContent_GetProperties(antlr4)
  if(NOT antlr4_POPULATED)
    FetchContent_Populate(antlr4)
    
    # Add ANTLR4 as a subdirectory
    set(ANTLR4_WITH_LIBCXX OFF)
    set(ANTLR4_INSTALL_GTEST OFF)
    set(ANTLR4_BUILD_CPP_TESTS OFF)
    set(ANTLR4_BUILD_RUNTIME ON)
    set(ANTLR4_BUILD_TOOL OFF)
    
    add_subdirectory(${antlr4_SOURCE_DIR}/runtime/Cpp ${antlr4_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  
  set(ANTLR4_INCLUDE_DIRS ${antlr4_SOURCE_DIR}/runtime/Cpp/runtime/src)
  set(ANTLR4_LIBRARIES antlr4_shared)
  
  # Create an imported target for ANTLR4
  if(NOT TARGET ANTLR4::ANTLR4)
    add_library(ANTLR4::ANTLR4 UNKNOWN IMPORTED)
    set_target_properties(ANTLR4::ANTLR4 PROPERTIES
      IMPORTED_LOCATION ${ANTLR4_LIBRARIES}
      INTERFACE_INCLUDE_DIRECTORIES "${ANTLR4_INCLUDE_DIRS}"
    )
    
    # Add system dependencies
    if(UNIX)
      find_package(Threads REQUIRED)
      set_property(TARGET ANTLR4::ANTLR4 APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "Threads::Threads"
      )
      
      if(CMAKE_DL_LIBS)
        set_property(TARGET ANTLR4::ANTLR4 APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS}
        )
      endif()
    endif()
  endif()
endif()

# Find or download Google Test (for testing)
if(BUILD_TESTS)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
    GIT_SHALLOW TRUE
  )
  
  # Only populate if not already populated
  FetchContent_GetProperties(googletest)
  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  
  # Create imported targets for GTest and GMock
  if(NOT TARGET GTest::GTest)
    add_library(GTest::GTest INTERFACE IMPORTED)
    target_link_libraries(GTest::GTest INTERFACE gtest)
    target_include_directories(GTest::GTest INTERFACE ${googletest_SOURCE_DIR}/googletest/include)
  endif()
  
  if(NOT TARGET GTest::Main)
    add_library(GTest::Main INTERFACE IMPORTED)
    target_link_libraries(GTest::Main INTERFACE gtest_main)
    target_include_directories(GTest::Main INTERFACE ${googletest_SOURCE_DIR}/googletest/include)
  endif()
  
  if(NOT TARGET GTest::GMock)
    add_library(GTest::GMock INTERFACE IMPORTED)
    target_link_libraries(GTest::GMock INTERFACE gmock)
    target_include_directories(GTest::GMock INTERFACE ${googletest_SOURCE_DIR}/googlemock/include)
  endif()
  
  if(NOT TARGET GTest::GMockMain)
    add_library(GTest::GMockMain INTERFACE IMPORTED)
    target_link_libraries(GTest::GMockMain INTERFACE gmock_main)
    target_include_directories(GTest::GMockMain INTERFACE ${googletest_SOURCE_DIR}/googlemock/include)
  endif()
endif()

# Find or download Google Benchmark (for benchmarking)
if(BUILD_BENCHMARKS)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.7.0
    GIT_SHALLOW TRUE
  )
  
  # Only populate if not already populated
  FetchContent_GetProperties(benchmark)
  if(NOT benchmark_POPULATED)
    FetchContent_Populate(benchmark)
    
    # Set benchmark options
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    
    add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  
  # Create an imported target for Google Benchmark
  if(NOT TARGET benchmark::benchmark)
    add_library(benchmark::benchmark INTERFACE IMPORTED)
    target_link_libraries(benchmark::benchmark INTERFACE benchmark::benchmark)
  endif()
endif()

# Create a summary of dependency information
include(FeatureSummary)
add_feature_info("System Dependencies" USE_SYSTEM_DEPS "Use system-installed dependencies")
add_feature_info("RocksDB" USE_SYSTEM_ROCKSDB "Use system-installed RocksDB (${KADEDB_ROCKSDB_VERSION})")
add_feature_info("ANTLR4" USE_SYSTEM_ANTLR "Use system-installed ANTLR4 (${KADEDB_ANTLR_VERSION})")
add_feature_info("OpenSSL" OPENSSL_FOUND "OpenSSL (${KADEDB_OPENSSL_VERSION})")

# Print a summary of the configuration
message(STATUS "")
message(STATUS "=========================================")
message(STATUS "KadeDB Dependency Configuration Summary")
message(STATUS "=========================================")
message(STATUS "System Dependencies: ${USE_SYSTEM_DEPS}")
message(STATUS "  - RocksDB: ${ROCKSDB_LIBRARIES} (${KADEDB_ROCKSDB_VERSION})")
message(STATUS "  - ANTLR4: ${ANTLR4_LIBRARIES} (${KADEDB_ANTLR_VERSION})")
message(STATUS "  - OpenSSL: ${OPENSSL_FOUND} (${KADEDB_OPENSSL_VERSION})")
message(STATUS "  - Build Tests: ${BUILD_TESTS}")
message(STATUS "  - Build Benchmarks: ${BUILD_BENCHMARKS}")
message(STATUS "=========================================")
message(STATUS "")
