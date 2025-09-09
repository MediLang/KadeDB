#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cstdio>
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

int main() {
  InMemoryRelationalStorage rs;
  auto schema = makePersonSchema();

  // Create table
  Status st = rs.createTable("person", schema);
  if (!st.ok()) {
    std::fprintf(stderr, "createTable failed: %s\n", st.message().c_str());
    return 1;
  }

  // Error path 1: Insert row missing non-nullable column "name"
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    // r.set(1, name) intentionally omitted
    r.set(2, ValueFactory::createInteger(30));
    st = rs.insertRow("person", r);
    if (!st.ok()) {
      std::printf("expected error (invalid schema): %s\n",
                  st.message().c_str());
    } else {
      std::fprintf(stderr, "ERROR: expected invalid schema failure\n");
      return 2;
    }
  }

  // Insert a valid row so we can test duplicate unique
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Ada"));
    r.set(2, ValueFactory::createInteger(36));
    st = rs.insertRow("person", r);
    if (!st.ok()) {
      std::fprintf(stderr, "unexpected insert error: %s\n",
                   st.message().c_str());
      return 3;
    }
  }

  // Error path 2: Duplicate unique id
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1)); // duplicate id
    r.set(1, ValueFactory::createString("Dup"));
    st = rs.insertRow("person", r);
    if (!st.ok()) {
      std::printf("expected error (duplicate unique): %s\n",
                  st.message().c_str());
    } else {
      std::fprintf(stderr, "ERROR: expected duplicate unique failure\n");
      return 4;
    }
  }

  // Demonstrate composite predicate (age > 30 AND name != "Ada")
  {
    Predicate gtAge;
    gtAge.kind = Predicate::Kind::Comparison;
    gtAge.column = "age";
    gtAge.op = Predicate::Op::Gt;
    gtAge.rhs = ValueFactory::createInteger(30);

    Predicate notNameAda;
    notNameAda.kind = Predicate::Kind::Comparison;
    notNameAda.column = "name";
    notNameAda.op = Predicate::Op::Ne;
    notNameAda.rhs = ValueFactory::createString("Ada");

    Predicate both;
    both.kind = Predicate::Kind::And;
    both.children.push_back(std::move(gtAge));
    both.children.push_back(std::move(notNameAda));

    std::optional<Predicate> where;
    where.emplace(std::move(both));
    auto res = rs.select("person", {"id", "name"}, where);
    if (!res.hasValue()) {
      std::fprintf(stderr, "select failed: %s\n",
                   res.status().message().c_str());
      return 5;
    }
    const ResultSet &s = res.value();
    std::printf("composite-predicate rows=%zu\n", s.rowCount());
  }

  std::puts("errors_example_done");
  return 0;
}
