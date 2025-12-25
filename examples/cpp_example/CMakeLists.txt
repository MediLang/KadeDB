add_executable(kadedb_cpp_example
  main.cpp
)

target_link_libraries(kadedb_cpp_example PRIVATE KadeDB::kadedb_core)

target_compile_features(kadedb_cpp_example PRIVATE cxx_std_17)
