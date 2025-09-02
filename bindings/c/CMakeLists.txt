# C ABI library
set(LIB_TYPE SHARED)
if(NOT KADEDB_BUILD_SHARED)
  set(LIB_TYPE STATIC)
endif()

add_library(kadedb_c ${LIB_TYPE}
  src/kadedb_c.cpp
  src/kadedb_ffi_helpers.cpp
  # src/examples.cpp  # Temporarily disabled until core FFI works
)

add_library(KadeDB::kadedb_c ALIAS kadedb_c)

target_include_directories(kadedb_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${KADEDB_GENERATED_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(kadedb_c
  PRIVATE KadeDB::kadedb_core
)

set_target_properties(kadedb_c PROPERTIES
  OUTPUT_NAME kadedb_c
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
)

if(MSVC)
  # Ensure a .lib import library is generated for the DLL on Windows
  set_target_properties(kadedb_c PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  # Silence secure CRT warnings for legacy-like C APIs
  target_compile_definitions(kadedb_c PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS kadedb_c
  EXPORT KadeDBTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
