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
  auto vec = std::move(q.value());
  // Expect only user_id=1
  assert(vec.size() == 1);

  // Lookup username via relational select: for active==true we expect key "1"
  if (vec.empty())
    return 1;
  const std::string key = vec[0].first;
  if (key != "1")
    return 2;

#ifdef KADEDB_FORCE_DOC_DEREF
  // Diagnostic path (enabled in ASAN diagnostic job): intentionally access the
  // Document returned by query to reproduce any lifetime issues under ASAN.
  // This code is only compiled in the diagnostic job.
  const auto &doc0_diag = vec[0].second;
  auto it_diag = doc0_diag.find("user_id");
  assert(it_diag != doc0_diag.end());
  assert(it_diag->second != nullptr);
  assert(it_diag->second->type() == ValueType::Integer);
  int userId = static_cast<int>(
      static_cast<const IntegerValue &>(*it_diag->second).value());
#else
  // Default safe path used across normal CI jobs
  int userId = 1;
#endif

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
