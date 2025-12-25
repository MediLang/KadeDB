add_executable(kadedb_lite_cli
  main.c
)

target_link_libraries(kadedb_lite_cli PRIVATE KadeDB::kadedb_lite)

target_compile_features(kadedb_lite_cli PRIVATE c_std_99)
