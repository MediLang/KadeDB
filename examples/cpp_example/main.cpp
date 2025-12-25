#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cstdio>
#include <optional>
#include <vector>

using namespace kadedb;

static TableSchema makePersonSchema() {
  std::vector<Column> cols;
  {
    Column c;
    c.name = "id";
    c.type = ColumnType::Integer;
    c.nullable = false;
    c.unique = true;
    cols.push_back(c);
  }
  {
    Column c;
    c.name = "name";
    c.type = ColumnType::String;
    c.nullable = false;
    c.unique = false;
    cols.push_back(c);
  }
  {
    Column c;
    c.name = "age";
    c.type = ColumnType::Integer;
    c.nullable = true;
    c.unique = false;
    cols.push_back(c);
  }

  return TableSchema(cols, std::optional<std::string>("id"));
}

int main() {
  InMemoryRelationalStorage storage;
  TableSchema schema = makePersonSchema();

  Status st = storage.createTable("person", schema);
  if (!st.ok()) {
    std::fprintf(stderr, "createTable failed: %s\n", st.message().c_str());
    return 1;
  }

  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Ada"));
    r.set(2, ValueFactory::createInteger(36));
    st = storage.insertRow("person", r);
    if (!st.ok()) {
      std::fprintf(stderr, "insertRow failed: %s\n", st.message().c_str());
      return 2;
    }
  }

  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(2));
    r.set(1, ValueFactory::createString("Grace"));
    r.set(2, nullptr);
    st = storage.insertRow("person", r);
    if (!st.ok()) {
      std::fprintf(stderr, "insertRow failed: %s\n", st.message().c_str());
      return 3;
    }
  }

  std::optional<Predicate> where;
  auto result = storage.select("person", {"id", "name", "age"}, where);
  if (!result.hasValue()) {
    std::fprintf(stderr, "select failed: %s\n",
                 result.status().message().c_str());
    return 4;
  }

  const ResultSet &rs = result.value();
  std::printf("Rows: %zu\n", rs.rowCount());

  for (size_t i = 0; i < rs.rowCount(); i++) {
    std::printf("  row[%zu]: ", i);
    for (size_t c = 0; c < rs.columnCount(); c++) {
      std::printf("%s", rs.at(i, c).toString().c_str());
      if (c + 1 < rs.columnCount())
        std::printf(", ");
    }
    std::printf("\n");
  }

  return 0;
}
