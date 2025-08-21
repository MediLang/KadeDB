# C ABI library
set(LIB_TYPE SHARED)
if(NOT KADEDB_BUILD_SHARED)
  set(LIB_TYPE STATIC)
endif()

add_library(kadedb_c ${LIB_TYPE}
  src/kadedb_c.cpp
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

install(TARGETS kadedb_c
  EXPORT KadeDBTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
