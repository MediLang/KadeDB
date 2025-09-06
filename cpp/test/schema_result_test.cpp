#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "kadedb/result.h"
#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

int main() {
  // Define schema
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
    c.name = "active";
    c.type = ColumnType::Boolean;
    c.nullable = true;
    cols.push_back(c);
  }
  TableSchema schema(cols, std::string("id"));

  assert(schema.findColumn("name") == 1);

  // Create a row matching schema
  Row row(schema.columns().size());
  row.set(0, std::make_unique<IntegerValue>(1));
  row.set(1, std::make_unique<StringValue>("alice"));
  row.set(2, std::make_unique<BooleanValue>(true));

  auto err = SchemaValidator::validateRow(schema, row);
  assert(err.empty());

  // Build a simple result set
  ResultSet rs({"id", "name", "active"},
               {ColumnType::Integer, ColumnType::String, ColumnType::Boolean});
  // Transfer values from Row to ResultRow clones for safety
  std::vector<std::unique_ptr<Value>> vals;
  vals.reserve(row.size());
  for (size_t i = 0; i < row.size(); ++i) {
    vals.emplace_back(row.at(i).clone());
  }
  rs.addRow(ResultRow(std::move(vals)));

  assert(rs.rowCount() == 1);
  rs.reset();
  // Ensure next() is executed even in Release (asserts may be compiled out)
  rs.next();
  const auto &r0 = rs.current();
  assert(static_cast<const IntegerValue &>(r0.at(0)).asInt() == 1);
  assert(static_cast<const StringValue &>(r0.at(1)).asString() ==
         std::string("alice"));
  assert(static_cast<const BooleanValue &>(r0.at(2)).asBool() == true);

  return 0;
}
