#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <optional>
#include <vector>

using namespace kadedb;

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

int main() {
  InMemoryRelationalStorage rs;
  auto schema = makePersonSchema();
  assert(rs.createTable("person", schema).ok());

  // Insert rows: (1, Ada, 36), (2, Grace, 41), (3, Bob, 29)
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Ada"));
    r.set(2, ValueFactory::createInteger(36));
    assert(rs.insertRow("person", r).ok());
  }
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(2));
    r.set(1, ValueFactory::createString("Grace"));
    r.set(2, ValueFactory::createInteger(41));
    assert(rs.insertRow("person", r).ok());
  }
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(3));
    r.set(1, ValueFactory::createString("Bob"));
    r.set(2, ValueFactory::createInteger(29));
    assert(rs.insertRow("person", r).ok());
  }

  // AND: age >= 30 AND age <= 40 -> Ada only
  {
    std::vector<Predicate> cs;
    cs.emplace_back(
        cmp("age", Predicate::Op::Ge, ValueFactory::createInteger(30)));
    cs.emplace_back(
        cmp("age", Predicate::Op::Le, ValueFactory::createInteger(40)));
    std::optional<Predicate> where;
    where.emplace(And(std::move(cs)));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 1);
  }

  // OR: name == Ada OR age < 30 -> Ada and Bob (2 rows)
  {
    std::vector<Predicate> cs;
    cs.emplace_back(
        cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada")));
    cs.emplace_back(
        cmp("age", Predicate::Op::Lt, ValueFactory::createInteger(30)));
    std::optional<Predicate> where;
    where.emplace(Or(std::move(cs)));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 2);
  }

  // Corner cases: empty children semantics
  // AND with zero children -> true (neutral), returns all rows
  {
    Predicate p;
    p.kind = Predicate::Kind::And;
    std::optional<Predicate> where;
    where.emplace(std::move(p));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 3);
  }
  // OR with zero children -> false (neutral), returns zero rows
  {
    Predicate p;
    p.kind = Predicate::Kind::Or;
    std::optional<Predicate> where;
    where.emplace(std::move(p));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 0);
  }
  // NOT with zero children -> false, returns zero rows
  {
    Predicate p;
    p.kind = Predicate::Kind::Not;
    std::optional<Predicate> where;
    where.emplace(std::move(p));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 0);
  }

  // NOT: NOT(name == Ada) -> Grace and Bob (2 rows)
  {
    std::optional<Predicate> where;
    where.emplace(
        Not(cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada"))));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 2);
  }

  // Nested: A AND (B OR NOT C)
  // A: age >= 30
  // B: name == Ada
  // C: age < 40
  // Rows:
  //  Ada(36): A true (36>=30) and (B true or NOT C -> NOT(36<40)=false) => true
  //  OR false => true => included Grace(41): A true and (B false or
  //  NOT(41<40)=true) => true => included Bob(29): A false => excluded
  {
    Predicate A =
        cmp("age", Predicate::Op::Ge, ValueFactory::createInteger(30));
    Predicate B =
        cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada"));
    Predicate C =
        cmp("age", Predicate::Op::Lt, ValueFactory::createInteger(40));
    std::vector<Predicate> inner;
    inner.emplace_back(std::move(B));
    inner.emplace_back(Not(std::move(C)));
    Predicate innerOr = Or(std::move(inner));
    std::vector<Predicate> outer;
    outer.emplace_back(std::move(A));
    outer.emplace_back(std::move(innerOr));
    std::optional<Predicate> where;
    where.emplace(And(std::move(outer)));
    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 2);
  }

  return 0;
}
