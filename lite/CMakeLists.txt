# KadeDB-Lite (C API) with optional RocksDB
set(LIB_TYPE SHARED)
if(NOT KADEDB_BUILD_SHARED)
  set(LIB_TYPE STATIC)
endif()

# Optional RocksDB integration
if(KADEDB_LITE_WITH_ROCKSDB)
  find_package(RocksDB QUIET CONFIG)
  if(RocksDB_FOUND)
    message(STATUS "RocksDB found: enabling integration")
  else()
    option(KADEDB_LITE_FETCH_ROCKSDB "Fetch RocksDB from source if not found" OFF)
    if(KADEDB_LITE_FETCH_ROCKSDB)
      message(STATUS "RocksDB not found: fetching via FetchContent")
      include(FetchContent)
      FetchContent_Declare(
        rocksdb
        URL https://github.com/facebook/rocksdb/archive/refs/tags/v8.11.3.tar.gz
        URL_HASH SHA256=1b20f1a7b9657d7b3e76082404ace9e0af0c6c35e5bcfa4a507d69bbcb32a976
      )
      # Configure RocksDB to a minimal build
      set(ROCKSDB_BUILD_SHARED ON CACHE BOOL "" FORCE)
      set(ROCKSDB_BUILD_STATIC OFF CACHE BOOL "" FORCE)
      set(WITH_TESTS OFF CACHE BOOL "" FORCE)
      set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
      set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
      set(ROCKSDB_DISABLE_GFLAGS ON CACHE BOOL "" FORCE)
      set(FAIL_ON_WARNINGS OFF CACHE BOOL "" FORCE)
      FetchContent_MakeAvailable(rocksdb)
      # After FetchContent, RocksDB might expose target "rocksdb"
      if(TARGET rocksdb)
        set(RocksDB_FOUND TRUE)
        set(KADEDB_ROCKSDB_TARGET rocksdb)
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
  else()
    message(WARNING "RocksDB found but no recognizable CMake target to link. Proceeding without linking.")
  endif()
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
