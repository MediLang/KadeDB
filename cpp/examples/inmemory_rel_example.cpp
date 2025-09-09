#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cstdio>
#include <optional>
#include <vector>

using namespace kadedb;

// Simple predicate builders to reduce boilerplate in the example
static Predicate cmp(const std::string &col, Predicate::Op op,
                     std::unique_ptr<Value> rhs) {
  Predicate p;
  p.kind = Predicate::Kind::Comparison;
  p.column = col;
  p.op = op;
  p.rhs = std::move(rhs);
  return p;
}

static Predicate And(std::vector<Predicate> cs) {
  Predicate p;
  p.kind = Predicate::Kind::And;
  p.children = std::move(cs);
  return p;
}

static Predicate Or(std::vector<Predicate> cs) {
  Predicate p;
  p.kind = Predicate::Kind::Or;
  p.children = std::move(cs);
  return p;
}

static Predicate Not(Predicate c) {
  Predicate p;
  p.kind = Predicate::Kind::Not;
  p.children.push_back(std::move(c));
  return p;
}

static TableSchema makePersonSchema() {
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
    c.name = "age";
    c.type = ColumnType::Integer;
    c.nullable = true;
    cols.push_back(c);
  }
  return TableSchema(cols, std::optional<std::string>("id"));
}

int main() {
  InMemoryRelationalStorage rs;
  auto schema = makePersonSchema();

  // Create table
  Status st = rs.createTable("person", schema);
  if (!st.ok()) {
    std::fprintf(stderr, "createTable failed: %s\n", st.message().c_str());
    return 1;
  }

  // Insert a couple rows
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Ada"));
    r.set(2, ValueFactory::createInteger(36));
    st = rs.insertRow("person", r);
    if (!st.ok()) {
      std::fprintf(stderr, "insertRow failed: %s\n", st.message().c_str());
      return 1;
    }
  }
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(2));
    r.set(1, ValueFactory::createString("Grace"));
    r.set(2, ValueFactory::createInteger(41));
    st = rs.insertRow("person", r);
    if (!st.ok()) {
      std::fprintf(stderr, "insertRow failed: %s\n", st.message().c_str());
      return 1;
    }
  }

  // SELECT name WHERE age > 36
  std::optional<Predicate> pred;
  pred.emplace();
  pred->column = "age";
  pred->op = Predicate::Op::Gt;
  pred->rhs = ValueFactory::createInteger(36);
  auto res = rs.select("person", {"name"}, pred);
  if (!res.hasValue()) {
    const auto &s = res.status();
    std::fprintf(stderr, "select error: %s\n", s.message().c_str());
    return 1;
  }

  const ResultSet &set = res.value();
  for (size_t i = 0; i < set.rowCount(); ++i) {
    std::printf("name=%s\n", set.at(i, 0).toString().c_str());
  }

  // UPDATE: set age = 42 where name == "Grace"
  {
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["age"] = ValueFactory::createInteger(42);
    std::optional<Predicate> where;
    where.emplace();
    where->column = "name";
    where->op = Predicate::Op::Eq;
    where->rhs = ValueFactory::createString("Grace");
    Status st_upd = rs.updateRows("person", assigns, where);
    if (!st_upd.ok()) {
      std::fprintf(stderr, "updateRows failed: %s\n", st_upd.message().c_str());
      return 1;
    }

    auto check = rs.select("person", {"name", "age"}, std::nullopt);
    if (!check.hasValue()) {
      std::fprintf(stderr, "post-update select failed: %s\n",
                   check.status().message().c_str());
      return 1;
    }
    const ResultSet &s2 = check.value();
    for (size_t i = 0; i < s2.rowCount(); ++i) {
      std::printf("row: name=%s age=%s\n", s2.at(i, 0).toString().c_str(),
                  s2.at(i, 1).toString().c_str());
    }
  }

  // DELETE: delete rows where age > 40
  {
    std::optional<Predicate> where;
    where.emplace();
    where->column = "age";
    where->op = Predicate::Op::Gt;
    where->rhs = ValueFactory::createInteger(40);
    auto del = rs.deleteRows("person", where);
    if (!del.hasValue()) {
      std::fprintf(stderr, "deleteRows failed: %s\n",
                   del.status().message().c_str());
      return 1;
    }
    std::printf("deleted=%zu\n", del.value());

    auto remaining = rs.select("person", {"id", "name", "age"}, std::nullopt);
    if (!remaining.hasValue()) {
      std::fprintf(stderr, "post-delete select failed: %s\n",
                   remaining.status().message().c_str());
      return 1;
    }
    const ResultSet &s3 = remaining.value();
    for (size_t i = 0; i < s3.rowCount(); ++i) {
      std::printf(
          "remaining: id=%s name=%s age=%s\n", s3.at(i, 0).toString().c_str(),
          s3.at(i, 1).toString().c_str(), s3.at(i, 2).toString().c_str());
    }
  }

  // OR: name == "Ada" OR age < 35
  {
    std::vector<Predicate> cs;
    cs.emplace_back(
        cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada")));
    cs.emplace_back(
        cmp("age", Predicate::Op::Lt, ValueFactory::createInteger(35)));
    Predicate p_or = Or(std::move(cs));
    std::optional<Predicate> where;
    where.emplace(std::move(p_or));
    auto res = rs.select("person", {"name", "age"}, where);
    if (!res.hasValue()) {
      std::fprintf(stderr, "OR select failed: %s\n",
                   res.status().message().c_str());
      return 1;
    }
    const ResultSet &s = res.value();
    std::printf("OR matched rows=%zu\n", s.rowCount());
  }

  // NOT: NOT(name == "Ada")
  {
    Predicate p_not =
        Not(cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada")));
    std::optional<Predicate> where;
    where.emplace(std::move(p_not));
    auto res = rs.select("person", {"name"}, where);
    if (!res.hasValue()) {
      std::fprintf(stderr, "NOT select failed: %s\n",
                   res.status().message().c_str());
      return 1;
    }
    const ResultSet &s = res.value();
    std::printf("NOT matched rows=%zu\n", s.rowCount());
  }

  // AND: age >= 30 AND age <= 42
  {
    std::vector<Predicate> cs;
    cs.emplace_back(
        cmp("age", Predicate::Op::Ge, ValueFactory::createInteger(30)));
    cs.emplace_back(
        cmp("age", Predicate::Op::Le, ValueFactory::createInteger(42)));
    Predicate p_and = And(std::move(cs));
    std::optional<Predicate> where;
    where.emplace(std::move(p_and));
    auto res = rs.select("person", {"name", "age"}, where);
    if (!res.hasValue()) {
      std::fprintf(stderr, "AND select failed: %s\n",
                   res.status().message().c_str());
      return 1;
    }
    const ResultSet &s = res.value();
    std::printf("AND matched rows=%zu\n", s.rowCount());
  }

  return 0;
}
