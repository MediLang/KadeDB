#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace kadedb;

static DocumentSchema makeDocSchema() {
  DocumentSchema ds;
  Column id{"id", ColumnType::Integer, false, true, {}};
  Column name{"name", ColumnType::String, false, false, {}};
  Column age{"age", ColumnType::Integer, true, false, {}};
  ds.addField(id);
  ds.addField(name);
  ds.addField(age);
  return ds;
}

int main() {
  InMemoryDocumentStorage ds;

  // Count/get on missing collection -> NotFound
  {
    auto c = ds.count("missing");
    assert(!c.hasValue());
    assert(c.status().code() == StatusCode::NotFound);
    auto g = ds.get("missing", "k");
    assert(!g.hasValue());
    assert(g.status().code() == StatusCode::NotFound);
  }

  // Erase on missing collection -> NotFound
  {
    auto st = ds.erase("missing", "k");
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  // Create with schema
  auto schema = makeDocSchema();
  assert(ds.createCollection("people", schema).ok());

  // put invalid doc (missing required field 'name') -> InvalidArgument
  {
    Document d;
    d["id"] = ValueFactory::createInteger(1);
    auto st = ds.put("people", "1", d);
    assert(!st.ok());
    assert(st.code() == StatusCode::InvalidArgument);
  }

  // Put valid docs
  {
    Document d;
    d["id"] = ValueFactory::createInteger(1);
    d["name"] = ValueFactory::createString("Ada");
    assert(ds.put("people", "1", d).ok());
  }
  {
    Document d;
    d["id"] = ValueFactory::createInteger(2);
    d["name"] = ValueFactory::createString("Grace");
    assert(ds.put("people", "2", d).ok());
  }

  // Projection unknown field -> InvalidArgument
  {
    auto res = ds.query("people", {"unknown"}, std::nullopt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // Predicate unknown field -> InvalidArgument
  {
    std::optional<DocPredicate> where;
    where.emplace();
    where->kind = DocPredicate::Kind::Comparison;
    where->field = "unknown";
    where->op = DocPredicate::Op::Eq;
    where->rhs = ValueFactory::createInteger(1);
    auto res = ds.query("people", {}, where);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // dropCollection on missing -> NotFound
  {
    auto st = ds.dropCollection("nope");
    assert(!st.ok());
    assert(st.code() == StatusCode::NotFound);
  }

  // erase existing, then get should be NotFound
  {
    assert(ds.erase("people", "1").ok());
    auto g = ds.get("people", "1");
    assert(!g.hasValue());
    assert(g.status().code() == StatusCode::NotFound);
  }

  return 0;
}
