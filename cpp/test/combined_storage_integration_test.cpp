#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace kadedb;

static TableSchema makeUserSchema() {
  std::vector<Column> cols;
  cols.push_back(Column{"id", ColumnType::Integer, false, true, {}});
  cols.push_back(Column{"username", ColumnType::String, false, false, {}});
  cols.push_back(Column{"email", ColumnType::String, true, true, {}});
  return TableSchema(cols, std::optional<std::string>("id"));
}

int main() {
  // Relational side keeps structured rows
  InMemoryRelationalStorage rs;
  auto userSchema = makeUserSchema();
  assert(rs.createTable("users", userSchema).ok());

  // Insert two users
  {
    Row r(userSchema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("ada"));
    r.set(2, ValueFactory::createString("ada@example.com"));
    assert(rs.insertRow("users", r).ok());
  }
  {
    Row r(userSchema.columns().size());
    r.set(0, ValueFactory::createInteger(2));
    r.set(1, ValueFactory::createString("grace"));
    r.set(2, ValueFactory::createString("grace@example.com"));
    assert(rs.insertRow("users", r).ok());
  }

  // Document side stores per-user profiles keyed by user id as string
  InMemoryDocumentStorage ds;
  assert(ds.createCollection("profiles", std::nullopt).ok());

  {
    Document d;
    d["user_id"] = ValueFactory::createInteger(1);
    d["bio"] = ValueFactory::createString("Pioneer");
    d["active"] = ValueFactory::createBoolean(true);
    assert(ds.put("profiles", "1", d).ok());
  }
  {
    Document d;
    d["user_id"] = ValueFactory::createInteger(2);
    d["bio"] = ValueFactory::createString("Legend");
    d["active"] = ValueFactory::createBoolean(false);
    assert(ds.put("profiles", "2", d).ok());
  }

  // Cross-check: fetch all active profiles then project usernames from
  // relational
  std::optional<DocPredicate> where;
  where.emplace();
  where->kind = DocPredicate::Kind::Comparison;
  where->field = "active";
  where->op = DocPredicate::Op::Eq;
  where->rhs = ValueFactory::createBoolean(true);
  auto q = ds.query("profiles", {}, where);
  assert(q.hasValue());
  const auto &vec = q.value();
  // Expect only user_id=1
  assert(vec.size() == 1);

  // Lookup username of that user via relational select with predicate
  auto it_uid = vec[0].second.find("user_id");
  assert(it_uid != vec[0].second.end());
  assert(it_uid->second != nullptr);
  assert(it_uid->second->type() == ValueType::Integer);
  int userId = static_cast<const IntegerValue &>(*it_uid->second).value();

  std::optional<Predicate> w2;
  w2.emplace();
  w2->kind = Predicate::Kind::Comparison;
  w2->column = "id";
  w2->op = Predicate::Op::Eq;
  w2->rhs = ValueFactory::createInteger(userId);
  auto res = rs.select("users", {"username"}, w2);
  assert(res.hasValue());
  const ResultSet &s = res.value();
  assert(s.rowCount() == 1);

  return 0;
}
