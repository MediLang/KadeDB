# C ABI tests for ffi helpers

add_executable(kadedb_c_ffi_helpers_roundtrip_test
  ffi_helpers_roundtrip_test.c
)

target_link_libraries(kadedb_c_ffi_helpers_roundtrip_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_ffi_helpers_roundtrip_test COMMAND kadedb_c_ffi_helpers_roundtrip_test)

add_executable(kadedb_c_ffi_helpers_edgecases_test
  ffi_helpers_edgecases_test.c
)

target_link_libraries(kadedb_c_ffi_helpers_edgecases_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_ffi_helpers_edgecases_test COMMAND kadedb_c_ffi_helpers_edgecases_test)

add_executable(kadedb_c_crud_bindings_test
  crud_bindings_test.c
)

target_link_libraries(kadedb_c_crud_bindings_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_crud_bindings_test COMMAND kadedb_c_crud_bindings_test)

add_executable(kadedb_c_resultset_access_test
  resultset_access_test.c
)

target_link_libraries(kadedb_c_resultset_access_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_resultset_access_test COMMAND kadedb_c_resultset_access_test)

add_executable(kadedb_c_error_cases_test
  c_error_cases_test.c
)

target_link_libraries(kadedb_c_error_cases_test PRIVATE KadeDB::kadedb_c)

add_test(NAME kadedb_c_error_cases_test COMMAND kadedb_c_error_cases_test)
