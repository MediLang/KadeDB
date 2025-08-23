# KadeDB-Lite (C API) with optional RocksDB
set(LIB_TYPE SHARED)
if(NOT KADEDB_BUILD_SHARED)
  set(LIB_TYPE STATIC)
endif()

# Optional RocksDB integration
if(KADEDB_LITE_WITH_ROCKSDB)
  # Try both Config and Module modes
  find_package(RocksDB QUIET)
  if(RocksDB_FOUND)
    message(STATUS "RocksDB found: enabling integration")
  else()
    option(KADEDB_LITE_FETCH_ROCKSDB "Fetch RocksDB from source if not found" OFF)
    if(KADEDB_LITE_FETCH_ROCKSDB)
      message(STATUS "RocksDB not found: fetching via FetchContent")
      include(FetchContent)
      FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.11.3
        GIT_SHALLOW TRUE
      )
      # Configure RocksDB to a minimal build
      set(ROCKSDB_BUILD_SHARED ON CACHE BOOL "" FORCE)
      set(ROCKSDB_BUILD_STATIC OFF CACHE BOOL "" FORCE)
      set(WITH_TESTS OFF CACHE BOOL "" FORCE)
      set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
      set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
      # Minimize external deps
      set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
      set(WITH_SNAPPY OFF CACHE BOOL "" FORCE)
      set(WITH_LZ4 OFF CACHE BOOL "" FORCE)
      set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
      set(WITH_ZSTD OFF CACHE BOOL "" FORCE)
      set(FAIL_ON_WARNINGS OFF CACHE BOOL "" FORCE)
      FetchContent_MakeAvailable(rocksdb)
      # After FetchContent, RocksDB might expose target "rocksdb"
      if(TARGET rocksdb)
        set(KADEDB_ROCKSDB_TARGET rocksdb)
        set(RocksDB_FOUND TRUE)
      elseif(TARGET rocksdb-shared)
        set(KADEDB_ROCKSDB_TARGET rocksdb-shared)
        set(RocksDB_FOUND TRUE)
      endif()
    else()
      message(STATUS "RocksDB not found and KADEDB_LITE_FETCH_ROCKSDB=OFF: building Lite without RocksDB")
    endif()
  endif()
endif()

add_library(kadedb_lite ${LIB_TYPE}
  src/kadedb_lite.c
)

add_library(KadeDB::kadedb_lite ALIAS kadedb_lite)

target_include_directories(kadedb_lite
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(RocksDB_FOUND)
  target_compile_definitions(kadedb_lite PRIVATE KADEDB_LITE_HAS_ROCKSDB=1)
  if(TARGET RocksDB::rocksdb)
    target_link_libraries(kadedb_lite PRIVATE RocksDB::rocksdb)
  elseif(DEFINED KADEDB_ROCKSDB_TARGET AND TARGET ${KADEDB_ROCKSDB_TARGET})
    target_link_libraries(kadedb_lite PRIVATE ${KADEDB_ROCKSDB_TARGET})
  elseif(DEFINED ROCKSDB_LIBRARIES)
    target_link_libraries(kadedb_lite PRIVATE ${ROCKSDB_LIBRARIES})
    if(DEFINED ROCKSDB_INCLUDE_DIRS)
      target_include_directories(kadedb_lite PRIVATE ${ROCKSDB_INCLUDE_DIRS})
    endif()
  else()
    message(WARNING "RocksDB found but no recognizable CMake target to link. Proceeding without linking.")
  endif()
  # Ensure we link with a C++ linker since RocksDB is C++
  set_property(TARGET kadedb_lite PROPERTY LINKER_LANGUAGE CXX)
endif()

set_target_properties(kadedb_lite PROPERTIES
  OUTPUT_NAME kadedb_lite
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
)

install(TARGETS kadedb_lite
  EXPORT KadeDBTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
