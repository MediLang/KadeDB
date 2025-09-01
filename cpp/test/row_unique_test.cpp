#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

int main() {
  TableSchema schema(
      {
          Column{"id", ColumnType::Integer, false, true},
          Column{"name", ColumnType::String, true, false},
      },
      "id");

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
