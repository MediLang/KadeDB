#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <thread>
#include <vector>

using namespace kadedb;

static TableSchema makeSchema() {
  std::vector<Column> cols;
  cols.push_back(Column{"id", ColumnType::Integer, false, true, {}});
  cols.push_back(Column{"name", ColumnType::String, false, false, {}});
  return TableSchema(cols, std::optional<std::string>("id"));
}

int main() {
  // Relational concurrent inserts and reads
  InMemoryRelationalStorage rs;
  auto schema = makeSchema();
  assert(rs.createTable("t", schema).ok());

  const int N = 100;
  std::thread writer([&]() {
    for (int i = 0; i < N; ++i) {
      Row r(schema.columns().size());
      r.set(0, ValueFactory::createInteger(i));
      r.set(1, ValueFactory::createString("v" + std::to_string(i)));
      auto st = rs.insertRow("t", r);
      assert(st.ok());
    }
  });

  std::thread reader([&]() {
    // spin-read while writer runs
    for (int k = 0; k < N; ++k) {
      auto res = rs.select("t", {}, std::nullopt);
      // select should either be NotFound (if table dropped) or Ok; here it's Ok
      assert(res.hasValue());
    }
  });

  writer.join();
  reader.join();

  // Final check
  auto res = rs.select("t", {}, std::nullopt);
  assert(res.hasValue());
  // Row count should be N
  assert(res.value().rowCount() == static_cast<size_t>(N));

  // Document concurrent puts/gets
  InMemoryDocumentStorage ds;
  const int M = 100;

  std::thread dwriter([&]() {
    for (int i = 0; i < M; ++i) {
      Document d;
      d["id"] = ValueFactory::createInteger(i);
      d["name"] = ValueFactory::createString("n" + std::to_string(i));
      assert(ds.put("coll", std::to_string(i), d).ok());
    }
  });

  std::thread dreader([&]() {
    for (int k = 0; k < M; ++k) {
      auto c = ds.count("coll");
      // While the writer is running, count should be Ok or NotFound if not
      // created yet
      if (!c.hasValue()) {
        assert(c.status().code() == StatusCode::NotFound);
      }
    }
  });

  dwriter.join();
  dreader.join();

  auto c = ds.count("coll");
  assert(c.hasValue());
  assert(c.value() == static_cast<size_t>(M));

  return 0;
}
