#.rst:
# FindRocksDB
# --------
#
# Find the native RocksDB includes and library.
#
# IMPORTED Targets
# ^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``RocksDB::RocksDB``,
# if RocksDB has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   ROCKSDB_INCLUDE_DIRS  - where to find rocksdb/db.h, etc.
#   ROCKSDB_LIBRARIES     - List of libraries when using rocksdb.
#   ROCKSDB_FOUND         - True if rocksdb found.
#   ROCKSDB_VERSION       - Version of rocksdb found.
#
# Hints
# ^^^^^
#
# A user may set ``ROCKSDB_ROOT`` to a rocksdb installation root to tell this
# module where to look.

find_package(PkgConfig QUIET)

# Lookup for pkg-config if needed
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_ROCKSDB QUIET rocksdb)
  if(PC_ROCKSDB_FOUND)
    set(ROCKSDB_VERSION ${PC_ROCKSDB_VERSION})
  endif()
endif()

find_path(ROCKSDB_INCLUDE_DIR
  NAMES rocksdb/db.h
  HINTS ${ROCKSDB_ROOT} ${PC_ROCKSDB_INCLUDEDIR} ${PC_ROCKSDB_INCLUDE_DIRS}
  PATH_SUFFIXES include
)

find_library(ROCKSDB_LIBRARY
  NAMES rocksdb
  HINTS ${ROCKSDB_ROOT} ${PC_ROCKSDB_LIBDIR} ${PC_ROCKSDB_LIBRARY_DIRS}
  PATH_SUFFIXES lib ${CMAKE_INSTALL_LIBDIR}
)

# Extract version from header if not found by pkg-config
if(NOT ROCKSDB_VERSION AND ROCKSDB_INCLUDE_DIR AND EXISTS "${ROCKSDB_INCLUDE_DIR}/rocksdb/version.h")
  file(STRINGS "${ROCKSDB_INCLUDE_DIR}/rocksdb/version.h" _version_str
       REGEX "#define ROCKSDB_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+")
  if(_version_str)
    string(REGEX REPLACE ".*MAJOR[ \t]+([0-9]+).*" "\\1" _major_ver "${_version_str}")
    string(REGEX REPLACE ".*MINOR[ \t]+([0-9]+).*" "\\1" _minor_ver "${_version_str}")
    string(REGEX REPLACE ".*PATCH[ \t]+([0-9]+).*" "\\1" _patch_ver "${_version_str}")
    set(ROCKSDB_VERSION "${_major_ver}.${_minor_ver}.${_patch_ver}")
  endif()
  unset(_version_str)
  unset(_major_ver)
  unset(_minor_ver)
  unset(_patch_ver)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB
  FOUND_VAR ROCKSDB_FOUND
  REQUIRED_VARS
    ROCKSDB_LIBRARY
    ROCKSDB_INCLUDE_DIR
  VERSION_VAR ROCKSDB_VERSION
)

if(ROCKSDB_FOUND)
  set(ROCKSDB_INCLUDE_DIRS ${ROCKSDB_INCLUDE_DIR})
  set(ROCKSDB_LIBRARIES ${ROCKSDB_LIBRARY})
  
  if(NOT TARGET RocksDB::RocksDB)
    add_library(RocksDB::RocksDB UNKNOWN IMPORTED)
    set_target_properties(RocksDB::RocksDB PROPERTIES
      IMPORTED_LOCATION "${ROCKSDB_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIR}"
      INTERFACE_COMPILE_DEFINITIONS "ROCKSDB_LIBRARY"
    )
    
    if(UNIX)
      set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "-Wl,--no-as-needed"
      )
    endif()
  endif()
  
  # Add system libraries that RocksDB depends on
  if(UNIX)
    find_package(Threads REQUIRED)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
    )
  endif()
  
  if(CMAKE_DL_LIBS)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES ${CMAKE_DL_LIBS}
    )
  endif()
  
  if(ZLIB_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
    )
  endif()
  
  if(SNAPPY_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES Snappy::snappy
    )
  endif()
  
  if(LZ4_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES LZ4::LZ4
    )
  endif()
  
  if(ZSTD_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES zstd::zstd
    )
  endif()
  
  if(NUMA_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES ${NUMA_LIBRARIES}
    )
  endif()
  
  if(TBB_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES TBB::tbb
    )
  endif()
  
  if(JEMALLOC_FOUND)
    set_property(TARGET RocksDB::RocksDB APPEND PROPERTY
      INTERFACE_LINK_LIBRARIES jemalloc::jemalloc
    )
  endif()
  
  mark_as_advanced(ROCKSDB_INCLUDE_DIR ROCKSDB_LIBRARY)
endif()
