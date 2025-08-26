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

# Document schema tests
add_executable(kadedb_document_schema_test
  document_schema_test.cpp
)

target_link_libraries(kadedb_document_schema_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_document_schema_test PRIVATE cxx_std_17)

add_test(NAME kadedb_document_schema_test COMMAND kadedb_document_schema_test)

# Row uniqueness tests
add_executable(kadedb_row_unique_test
  row_unique_test.cpp
)

target_link_libraries(kadedb_row_unique_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_row_unique_test PRIVATE cxx_std_17)

add_test(NAME kadedb_row_unique_test COMMAND kadedb_row_unique_test)

# Constraint boundary tests
add_executable(kadedb_constraints_test
  constraints_test.cpp
)

target_link_libraries(kadedb_constraints_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_constraints_test PRIVATE cxx_std_17)

add_test(NAME kadedb_constraints_test COMMAND kadedb_constraints_test)

# C API constraints tests
add_executable(kadedb_c_api_constraints_test
  c_api_constraints_test.c
)

target_link_libraries(kadedb_c_api_constraints_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_api_constraints_test COMMAND kadedb_c_api_constraints_test)

add_executable(kadedb_c_api_table_constraints_test
  c_api_table_constraints_test.c
)

target_link_libraries(kadedb_c_api_table_constraints_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_api_table_constraints_test COMMAND kadedb_c_api_table_constraints_test)

add_executable(kadedb_c_api_table_schema_wrapper_test
  c_api_table_schema_wrapper_test.c
)

target_link_libraries(kadedb_c_api_table_schema_wrapper_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_api_table_schema_wrapper_test COMMAND kadedb_c_api_table_schema_wrapper_test)
