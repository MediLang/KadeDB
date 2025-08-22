# Test targets for KadeDB

add_executable(kadedb_core_smoke_test
  smoke_test.cpp
)

target_link_libraries(kadedb_core_smoke_test PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_core_smoke_test PRIVATE cxx_std_17)

add_test(NAME kadedb_core_smoke_test COMMAND kadedb_core_smoke_test)
