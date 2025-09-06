#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

static std::unique_ptr<Value> S(const std::string &s) {
  return std::make_unique<StringValue>(s);
}
static std::unique_ptr<Value> I(long long v) {
  return std::make_unique<IntegerValue>(v);
}
// static std::unique_ptr<Value> F(double v) {
//   return std::make_unique<FloatValue>(v);
// }

int main() {
  // String constraints: min/max length and oneOf
  {
    DocumentSchema ds;
    Column c;
    c.name = "status";
    c.type = ColumnType::String;
    c.nullable = false;
    c.unique = false;
    c.constraints.minLength = 2;
    c.constraints.maxLength = 4;
    c.constraints.oneOf = {"ok", "warn"};
    ds.addField(c);

    Document d1;
    d1.emplace("status", S("ok"));
    Document d2;
    d2.emplace("status", S("toolong"));
    Document d3;
    d3.emplace("status", S("no")); // not in oneOf

    auto e1 = SchemaValidator::validateDocument(ds, d1);
    assert(e1.empty());
    auto e2 = SchemaValidator::validateDocument(ds, d2);
    assert(!e2.empty());
    auto e3 = SchemaValidator::validateDocument(ds, d3);
    assert(!e3.empty());
  }

  // Numeric constraints: min/max values
  {
    DocumentSchema ds;
    Column c;
    c.name = "age";
    c.type = ColumnType::Integer;
    c.nullable = false;
    c.unique = false;
    c.constraints.minValue = 18;
    c.constraints.maxValue = 65;
    ds.addField(c);

    Document d1;
    d1.emplace("age", I(30));
    Document d2;
    d2.emplace("age", I(10));
    Document d3;
    d3.emplace("age", I(80));

    assert(SchemaValidator::validateDocument(ds, d1).empty());
    assert(!SchemaValidator::validateDocument(ds, d2).empty());
    assert(!SchemaValidator::validateDocument(ds, d3).empty());
  }

  // Uniqueness with ignoreNulls toggle on rows
  {
    std::vector<Column> cols;
    {
      Column c;
      c.name = "id";
      c.type = ColumnType::Integer;
      c.nullable = true;
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
    TableSchema ts(cols);

    std::vector<Row> rows;
    // row0: id=null
    rows.emplace_back(Row(2));
    rows.back().set(0, nullptr);
    rows.back().set(1, S("a"));
    // row1: id=null
    rows.emplace_back(Row(2));
    rows.back().set(0, nullptr);
    rows.back().set(1, S("b"));

    // ignoreNulls=true => OK
    auto e1 = SchemaValidator::validateUnique(ts, rows, true);
    assert(e1.empty());

    // ignoreNulls=false => should fail due to duplicate nulls in unique column
    auto e2 = SchemaValidator::validateUnique(ts, rows, false);
    assert(!e2.empty());
  }

  return 0;
}
