# Core C++ library
set(LIB_TYPE SHARED)
if(NOT KADEDB_BUILD_SHARED)
  set(LIB_TYPE STATIC)
endif()

add_library(kadedb_core ${LIB_TYPE}
  src/core/version.cpp
  src/core/value.cpp
  src/core/schema.cpp
)

add_library(KadeDB::kadedb_core ALIAS kadedb_core)

# Include directories
# - Public headers in source tree
# - Generated headers in build tree
# - Installed headers under include/

target_include_directories(kadedb_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${KADEDB_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Compile features
target_compile_features(kadedb_core PUBLIC cxx_std_17)

# Properties
set_target_properties(kadedb_core PROPERTIES
  OUTPUT_NAME kadedb_core
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
)

# Compile features/definitions
if(MSVC)
  target_compile_definitions(kadedb_core PRIVATE _CRT_SECURE_NO_WARNINGS)
  set_target_properties(kadedb_core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Install
install(TARGETS kadedb_core
  EXPORT KadeDBTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
