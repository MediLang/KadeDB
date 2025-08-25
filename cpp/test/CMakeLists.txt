# Test targets for KadeDB

add_executable(kadedb_core_smoke_test
  smoke_test.cpp
)

target_link_libraries(kadedb_core_smoke_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_core_smoke_test PRIVATE cxx_std_17)

add_test(NAME kadedb_core_smoke_test COMMAND kadedb_core_smoke_test)

# Value hierarchy tests
add_executable(kadedb_value_test
  value_test.cpp
)

target_link_libraries(kadedb_value_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_value_test PRIVATE cxx_std_17)

add_test(NAME kadedb_value_test COMMAND kadedb_value_test)

# Schema and Result tests
add_executable(kadedb_schema_result_test
  schema_result_test.cpp
)

target_link_libraries(kadedb_schema_result_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_schema_result_test PRIVATE cxx_std_17)

add_test(NAME kadedb_schema_result_test COMMAND kadedb_schema_result_test)
