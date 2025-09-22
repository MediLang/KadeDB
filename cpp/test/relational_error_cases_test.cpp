#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <optional>
#include <unordered_map>
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
    cols.push_back(c);
  }
  {
    Column c;
    c.name = "age";
    c.type = ColumnType::Integer;
    c.nullable = true;
    cols.push_back(c);
  }
  return TableSchema(cols, std::optional<std::string>("id"));
}

int main() {
  InMemoryRelationalStorage rs;

  // insertRow on missing table -> NotFound
  {
    Row r(2);
    auto st = rs.insertRow("missing", r);
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  auto schema = makePersonSchema();
  assert(rs.createTable("person", schema).ok());

  // select projection unknown column -> InvalidArgument
  {
    auto res = rs.select("person", {"unknown"}, std::nullopt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // updateRows on missing table -> NotFound
  {
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["name"] = ValueFactory::createString("X");
    auto st = rs.updateRows("missing", assigns, std::nullopt);
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  // deleteRows on missing table -> NotFound
  {
    auto res = rs.deleteRows("missing", std::nullopt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::NotFound);
  }

  // truncateTable on missing table -> NotFound
  {
    auto st = rs.truncateTable("missing");
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  // dropTable on missing table -> NotFound
  {
    auto st = rs.dropTable("missing");
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  return 0;
}
