# Lite tests

add_executable(kadedb_lite_smoke_test
  lite_smoke_test.c
)

target_link_libraries(kadedb_lite_smoke_test PRIVATE KadeDB::kadedb_lite)

target_compile_features(kadedb_lite_smoke_test PRIVATE c_std_99)

add_test(NAME kadedb_lite_smoke_test COMMAND kadedb_lite_smoke_test)

add_executable(kadedb_lite_kv_test
  lite_kv_test.c
)

target_link_libraries(kadedb_lite_kv_test PRIVATE KadeDB::kadedb_lite)

target_compile_features(kadedb_lite_kv_test PRIVATE c_std_99)

add_test(NAME kadedb_lite_kv_test COMMAND kadedb_lite_kv_test)

add_executable(kadedb_lite_query_test
  lite_query_test.c
)

target_link_libraries(kadedb_lite_query_test PRIVATE KadeDB::kadedb_lite)

target_compile_features(kadedb_lite_query_test PRIVATE c_std_99)

add_test(NAME kadedb_lite_query_test COMMAND kadedb_lite_query_test)

add_executable(kadedb_lite_sync_test
  lite_sync_test.c
)

target_link_libraries(kadedb_lite_sync_test PRIVATE KadeDB::kadedb_lite)

target_compile_features(kadedb_lite_sync_test PRIVATE c_std_99)

add_test(NAME kadedb_lite_sync_test COMMAND kadedb_lite_sync_test)
