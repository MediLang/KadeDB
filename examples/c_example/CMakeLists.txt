add_executable(kadedb_c_example
  main.c
)

target_link_libraries(kadedb_c_example PRIVATE KadeDB::kadedb_c)

target_compile_features(kadedb_c_example PRIVATE c_std_99)
