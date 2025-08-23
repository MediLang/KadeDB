#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KadeDB::kadedb_core" for configuration "Debug"
set_property(TARGET KadeDB::kadedb_core APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KadeDB::kadedb_core PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libkadedb_core.so.0.1.0"
  IMPORTED_SONAME_DEBUG "libkadedb_core.so.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS KadeDB::kadedb_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_KadeDB::kadedb_core "${_IMPORT_PREFIX}/lib/libkadedb_core.so.0.1.0" )

# Import target "KadeDB::kadedb_c" for configuration "Debug"
set_property(TARGET KadeDB::kadedb_c APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KadeDB::kadedb_c PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "KadeDB::kadedb_core"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libkadedb_c.so.0.1.0"
  IMPORTED_SONAME_DEBUG "libkadedb_c.so.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS KadeDB::kadedb_c )
list(APPEND _IMPORT_CHECK_FILES_FOR_KadeDB::kadedb_c "${_IMPORT_PREFIX}/lib/libkadedb_c.so.0.1.0" )

# Import target "KadeDB::kadedb_lite" for configuration "Debug"
set_property(TARGET KadeDB::kadedb_lite APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KadeDB::kadedb_lite PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libkadedb_lite.so.0.1.0"
  IMPORTED_SONAME_DEBUG "libkadedb_lite.so.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS KadeDB::kadedb_lite )
list(APPEND _IMPORT_CHECK_FILES_FOR_KadeDB::kadedb_lite "${_IMPORT_PREFIX}/lib/libkadedb_lite.so.0.1.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
