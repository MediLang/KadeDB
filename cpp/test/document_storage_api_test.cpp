#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

using namespace kadedb;

static DocumentSchema makeDocSchema() {
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
    c.name = "age";
    c.type = ColumnType::Integer;
    c.nullable = true;
    ds.addField(c);
  }
  return ds;
}

int main() {
  InMemoryDocumentStorage ds;

  // Initially no collections
  {
    auto names = ds.listCollections();
    assert(names.empty());
  }

  // Create a collection with schema
  auto schema = makeDocSchema();
  {
    auto st = ds.createCollection("people", schema);
    assert(st.ok());
    // Duplicate should fail
    auto st2 = ds.createCollection("people", schema);
    assert(!st2.ok());
  }

  // listCollections should contain 'people'
  {
    auto names = ds.listCollections();
    bool found = false;
    for (const auto &n : names)
      if (n == std::string("people")) {
        found = true;
        break;
      }
    assert(found);
  }

  // put valid doc
  {
    Document d;
    d["id"] = ValueFactory::createInteger(1);
    d["name"] = ValueFactory::createString("Ada");
    d["age"] = ValueFactory::createInteger(36);
    auto st = ds.put("people", "1", d);
    assert(st.ok());
  }

  // get existing
  {
    auto g = ds.get("people", "1");
    assert(g.hasValue());
    assert(g.value().at("name")->type() == ValueType::String);
  }

  // validation failure: missing required field 'name'
  {
    Document d;
    d["id"] = ValueFactory::createInteger(2);
    auto st = ds.put("people", "2", d);
    assert(!st.ok());
    assert(st.code() == StatusCode::InvalidArgument);
  }

  // Insert another valid document
  {
    Document d;
    d["id"] = ValueFactory::createInteger(2);
    d["name"] = ValueFactory::createString("Grace");
    auto st = ds.put("people", "2", d);
    assert(st.ok());
  }

  // Uniqueness violation on field 'id'
  {
    Document d;
    d["id"] = ValueFactory::createInteger(1); // duplicate id
    d["name"] = ValueFactory::createString("Dup");
    auto st = ds.put("people", "dup", d);
    assert(!st.ok());
    assert(st.code() == StatusCode::FailedPrecondition);
  }

  // count
  {
    auto c = ds.count("people");
    assert(c.hasValue());
    assert(c.value() == static_cast<size_t>(2));
  }

  // query: projection and predicate age > 35
  {
    std::optional<DocPredicate> where;
    where.emplace();
    where->field = "age";
    where->op = DocPredicate::Op::Gt;
    where->rhs = ValueFactory::createInteger(35);
    auto res = ds.query("people", {"name"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    // Ada (36) matches, Grace has no age (null) and should not match (MVP
    // semantics)
    assert(vec.size() == 1);
    assert(vec[0].second.find("name") != vec[0].second.end());
    assert(vec[0].second.size() == 1); // projection applied
  }

  // erase and recount
  {
    auto st = ds.erase("people", "1");
    assert(st.ok());
    auto c2 = ds.count("people");
    assert(c2.hasValue());
    assert(c2.value() == static_cast<size_t>(1));
  }

  // dropCollection and ensure NotFound afterward
  {
    auto st = ds.dropCollection("people");
    assert(st.ok());
    auto res = ds.get("people", "2");
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::NotFound);
  }

  // put on missing collection should auto-create (MVP behavior)
  {
    Document d;
    d["id"] = ValueFactory::createInteger(10);
    d["name"] = ValueFactory::createString("Auto");
    auto st = ds.put("auto", "10", d);
    assert(st.ok());
    // verify exists
    auto g = ds.get("auto", "10");
    assert(g.hasValue());
  }

  return 0;
}
