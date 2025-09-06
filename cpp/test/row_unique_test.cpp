#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

int main() {
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
    c.nullable = true;
    c.unique = false;
    cols.push_back(c);
  }
  TableSchema schema(cols, std::string("id"));

  // Build rows
  std::vector<Row> rows;
  rows.emplace_back(Row(schema.columns().size()));
  rows.back().set(0, std::make_unique<IntegerValue>(1));
  rows.back().set(1, std::make_unique<StringValue>(std::string{"a"}));

  rows.emplace_back(Row(schema.columns().size()));
  rows.back().set(0, std::make_unique<IntegerValue>(2));
  rows.back().set(1, std::make_unique<StringValue>(std::string{"b"}));

  // Validate uniqueness OK
  auto err = SchemaValidator::validateUnique(schema, rows);
  assert(err.empty());

  // Duplicate id should fail
  rows.emplace_back(Row(schema.columns().size()));
  rows.back().set(0, std::make_unique<IntegerValue>(2));
  rows.back().set(1, std::make_unique<StringValue>(std::string{"c"}));

  auto err2 = SchemaValidator::validateUnique(schema, rows);
  assert(!err2.empty());

  return 0;
}
