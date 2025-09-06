#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

int main() {
  // Build a document schema
  DocumentSchema ds;
  {
    Column c;
    c.name = "id";
    c.type = ColumnType::Integer;
    c.nullable = false;
    c.unique = true;
    ds.addField(c);
  }
  {
    Column c;
    c.name = "name";
    c.type = ColumnType::String;
    c.nullable = false;
    ds.addField(c);
  }
  {
    Column c;
    c.name = "active";
    c.type = ColumnType::Boolean;
    c.nullable = true;
    ds.addField(c);
  }

  // Valid document
  Document d1;
  d1["id"] = std::make_unique<IntegerValue>(42);
  d1["name"] = std::make_unique<StringValue>(std::string{"bob"});
  d1["active"] = std::make_unique<BooleanValue>(true);

  auto err = SchemaValidator::validateDocument(ds, d1);
  assert(err.empty());

  // Missing required field
  Document d2;
  d2["id"] = std::make_unique<IntegerValue>(43);
  auto err2 = SchemaValidator::validateDocument(ds, d2);
  assert(!err2.empty());

  // Unknown fields allowed
  Document d3;
  d3["id"] = std::make_unique<IntegerValue>(44);
  d3["name"] = std::make_unique<StringValue>(std::string{"carol"});
  d3["unknown"] = std::make_unique<StringValue>(std::string{"x"});
  auto err3 = SchemaValidator::validateDocument(ds, d3);
  assert(err3.empty());

  // Uniqueness checks
  std::vector<Document> docs;
  // Reserve to avoid reallocation (which would require copying Documents)
  docs.reserve(3);
  docs.emplace_back();
  docs.back()["id"] = std::make_unique<IntegerValue>(1);
  docs.back()["name"] = std::make_unique<StringValue>(std::string{"a"});
  docs.emplace_back();
  docs.back()["id"] = std::make_unique<IntegerValue>(2);
  docs.back()["name"] = std::make_unique<StringValue>(std::string{"b"});

  auto err4 = SchemaValidator::validateUnique(ds, docs);
  assert(err4.empty());

  // Duplicate id
  docs.emplace_back();
  docs.back()["id"] = std::make_unique<IntegerValue>(2);
  docs.back()["name"] = std::make_unique<StringValue>(std::string{"c"});
  auto err5 = SchemaValidator::validateUnique(ds, docs);
  assert(!err5.empty());

  return 0;
}
