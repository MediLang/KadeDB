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

# Result utilities (CSV/JSON/Pagination) tests
add_executable(kadedb_result_utils_test
  result_utils_test.cpp
)

target_link_libraries(kadedb_result_utils_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_result_utils_test PRIVATE cxx_std_17)

add_test(NAME kadedb_result_utils_test COMMAND kadedb_result_utils_test)

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

# Copy/Move semantics tests
add_executable(kadedb_copy_move_test
  copy_move_test.cpp
)

target_link_libraries(kadedb_copy_move_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_copy_move_test PRIVATE cxx_std_17)

add_test(NAME kadedb_copy_move_test COMMAND kadedb_copy_move_test)

# RowShallow shallow copy semantics tests
add_executable(kadedb_row_shallow_test
  row_shallow_test.cpp
)

target_link_libraries(kadedb_row_shallow_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_row_shallow_test PRIVATE cxx_std_17)

add_test(NAME kadedb_row_shallow_test COMMAND kadedb_row_shallow_test)

# Storage API tests
add_executable(kadedb_storage_api_test
  storage_api_test.cpp
)

target_link_libraries(kadedb_storage_api_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_storage_api_test PRIVATE cxx_std_17)

add_test(NAME kadedb_storage_api_test COMMAND kadedb_storage_api_test)

# Document storage API tests
add_executable(kadedb_document_storage_api_test
  document_storage_api_test.cpp
)

target_link_libraries(kadedb_document_storage_api_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_document_storage_api_test PRIVATE cxx_std_17)

add_test(NAME kadedb_document_storage_api_test COMMAND kadedb_document_storage_api_test)

# Composite predicate semantics tests
add_executable(kadedb_storage_predicates_test
  storage_predicates_test.cpp
)

target_link_libraries(kadedb_storage_predicates_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_storage_predicates_test PRIVATE cxx_std_17)

add_test(NAME kadedb_storage_predicates_test COMMAND kadedb_storage_predicates_test)

add_executable(kadedb_document_predicates_test
  document_predicates_test.cpp
)

target_link_libraries(kadedb_document_predicates_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_document_predicates_test PRIVATE cxx_std_17)

add_test(NAME kadedb_document_predicates_test COMMAND kadedb_document_predicates_test)

# Relational error cases tests
add_executable(kadedb_relational_error_cases_test
  relational_error_cases_test.cpp
)

target_link_libraries(kadedb_relational_error_cases_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_relational_error_cases_test PRIVATE cxx_std_17)

add_test(NAME kadedb_relational_error_cases_test COMMAND kadedb_relational_error_cases_test)

# Document error cases tests
add_executable(kadedb_document_error_cases_test
  document_error_cases_test.cpp
)

target_link_libraries(kadedb_document_error_cases_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_document_error_cases_test PRIVATE cxx_std_17)

add_test(NAME kadedb_document_error_cases_test COMMAND kadedb_document_error_cases_test)

# Combined storage integration test
add_executable(kadedb_combined_storage_integration_test
  combined_storage_integration_test.cpp
)

target_link_libraries(kadedb_combined_storage_integration_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_combined_storage_integration_test PRIVATE cxx_std_17)

add_test(NAME kadedb_combined_storage_integration_test COMMAND kadedb_combined_storage_integration_test)

# Basic concurrency tests (in-memory thread-safety)
add_executable(kadedb_concurrency_test
  concurrency_test.cpp
)

target_link_libraries(kadedb_concurrency_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_concurrency_test PRIVATE cxx_std_17)

add_test(NAME kadedb_concurrency_test COMMAND kadedb_concurrency_test)

# KadeQL parser tests
add_executable(kadedb_kadeql_test
  kadeql_test.cpp
)

target_link_libraries(kadedb_kadeql_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_test COMMAND kadedb_kadeql_test)

# KadeQL executor integration test (parser + executor + storage)
add_executable(kadedb_kadeql_executor_integration_test
  kadeql_executor_integration_test.cpp
)

target_link_libraries(kadedb_kadeql_executor_integration_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_executor_integration_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_executor_integration_test COMMAND kadedb_kadeql_executor_integration_test)

# KadeQL UPDATE/DELETE executor integration test
add_executable(kadedb_kadeql_update_delete_integration_test
  kadeql_update_delete_integration_test.cpp
)

target_link_libraries(kadedb_kadeql_update_delete_integration_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_update_delete_integration_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_update_delete_integration_test COMMAND kadedb_kadeql_update_delete_integration_test)

# KadeQL NOT predicates tests
add_executable(kadedb_kadeql_not_predicates_test
  kadeql_not_predicates_test.cpp
)

target_link_libraries(kadedb_kadeql_not_predicates_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_not_predicates_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_not_predicates_test COMMAND kadedb_kadeql_not_predicates_test)

# KadeQL computed UPDATE tests
add_executable(kadedb_kadeql_computed_update_test
  kadeql_computed_update_test.cpp
)

target_link_libraries(kadedb_kadeql_computed_update_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_computed_update_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_computed_update_test COMMAND kadedb_kadeql_computed_update_test)

# KadeQL mixed AND/OR, projection error, and reversed literal/identifier test
add_executable(kadedb_kadeql_mixed_projection_reversed_test
  kadeql_mixed_projection_reversed_test.cpp
)

target_link_libraries(kadedb_kadeql_mixed_projection_reversed_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_mixed_projection_reversed_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_mixed_projection_reversed_test COMMAND kadedb_kadeql_mixed_projection_reversed_test)

# KadeQL INSERT integration tests
add_executable(kadedb_kadeql_insert_integration_test
  kadeql_insert_integration_test.cpp
)

target_link_libraries(kadedb_kadeql_insert_integration_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_insert_integration_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_insert_integration_test COMMAND kadedb_kadeql_insert_integration_test)

# KadeQL optimizer canonicalization tests
add_executable(kadedb_kadeql_optimizer_canonicalization_test
  kadeql_optimizer_canonicalization_test.cpp
)

target_link_libraries(kadedb_kadeql_optimizer_canonicalization_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_optimizer_canonicalization_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_optimizer_canonicalization_test COMMAND kadedb_kadeql_optimizer_canonicalization_test)

# KadeQL unknown column predicate validation test
add_executable(kadedb_kadeql_unknown_column_predicate_test
  kadeql_unknown_column_predicate_test.cpp
)

target_link_libraries(kadedb_kadeql_unknown_column_predicate_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_unknown_column_predicate_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_unknown_column_predicate_test COMMAND kadedb_kadeql_unknown_column_predicate_test)

# KadeQL end-to-end tests (parser + executor + storage, error handling)
add_executable(kadedb_kadeql_end_to_end_test
  kadeql_end_to_end_test.cpp
)

target_link_libraries(kadedb_kadeql_end_to_end_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_end_to_end_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_end_to_end_test COMMAND kadedb_kadeql_end_to_end_test)

# KadeQL aggregation tests (TIME_BUCKET, FIRST, LAST)
add_executable(kadedb_kadeql_aggregation_test
  kadeql_aggregation_test.cpp
)

target_link_libraries(kadedb_kadeql_aggregation_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_aggregation_test PRIVATE cxx_std_17)

add_test(NAME kadedb_kadeql_aggregation_test COMMAND kadedb_kadeql_aggregation_test)

# Basic performance benchmark (not a pass/fail test, but useful in CI logs)
add_executable(kadedb_kadeql_basic_bench
  kadeql_basic_bench.cpp
)

target_link_libraries(kadedb_kadeql_basic_bench PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_kadeql_basic_bench PRIVATE cxx_std_17)

# Register as a test so it runs in CI and prints timing to stdout
add_test(NAME kadedb_kadeql_basic_bench COMMAND kadedb_kadeql_basic_bench)
