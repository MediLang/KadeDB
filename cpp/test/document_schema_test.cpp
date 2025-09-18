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

  // Uniqueness checks (use pointer-based API to avoid moving Documents)
  Document ud1; // unique docs container
  ud1["id"] = std::make_unique<IntegerValue>(1);
  ud1["name"] = std::make_unique<StringValue>(std::string{"a"});
  Document ud2;
  ud2["id"] = std::make_unique<IntegerValue>(2);
  ud2["name"] = std::make_unique<StringValue>(std::string{"b"});

  std::vector<const Document *> docPtrs;
  docPtrs.reserve(3);
  docPtrs.push_back(&ud1);
  docPtrs.push_back(&ud2);

  auto err4 = SchemaValidator::validateUnique(ds, docPtrs);
  assert(err4.empty());

  // Duplicate id
  Document ud3;
  ud3["id"] = std::make_unique<IntegerValue>(2);
  ud3["name"] = std::make_unique<StringValue>(std::string{"c"});
  docPtrs.push_back(&ud3);
  auto err5 = SchemaValidator::validateUnique(ds, docPtrs);
  assert(!err5.empty());

  return 0;
}
