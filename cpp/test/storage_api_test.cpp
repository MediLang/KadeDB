#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <iostream>

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
  // Relational tests
  InMemoryRelationalStorage rs;
  auto schema = makePersonSchema();

  // Create table
  {
    auto st = rs.createTable("person", schema);
    assert(st.ok());
    // Duplicate should fail
    auto st2 = rs.createTable("person", schema);
    assert(!st2.ok());
  }

  // Insert valid rows
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Ada"));
    r.set(2, ValueFactory::createInteger(36));
    auto st = rs.insertRow("person", r);
    assert(st.ok());
  }
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(2));
    r.set(1, ValueFactory::createString("Grace"));
    r.set(2, ValueFactory::createInteger(40));
    auto st = rs.insertRow("person", r);
    assert(st.ok());
  }

  // Insert duplicate unique id should fail
  {
    Row r(schema.columns().size());
    r.set(0, ValueFactory::createInteger(1));
    r.set(1, ValueFactory::createString("Duplicate"));
    auto st = rs.insertRow("person", r);
    assert(!st.ok());
  }

  // Select *
  {
    auto res = rs.select("person", {}, std::nullopt);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 2);
    // Projected columns: id, name, age
    assert(s.columnCount() == 3);
  }

  // Select projection and predicate
  {
    std::optional<Predicate> where;
    where.emplace();
    where->column = "age";
    where->op = Predicate::Op::Gt;
    where->rhs = ValueFactory::createInteger(36);

    auto res = rs.select("person", {"name"}, where);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 1);
    assert(s.columnCount() == 1);
    assert(s.at(0, 0).type() == ValueType::String);
  }

  // Unknown table returns Status::NotFound
  {
    auto res = rs.select("unknown", {}, std::nullopt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::NotFound);
  }

  // listTables should include 'person'
  {
    auto names = rs.listTables();
    bool found = false;
    for (const auto &n : names) {
      if (n == std::string("person")) {
        found = true;
        break;
      }
    }
    assert(found);
  }

  // updateRows: set age=41 where name=="Grace"
  {
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["age"] = ValueFactory::createInteger(41);
    std::optional<Predicate> where;
    where.emplace();
    where->column = "name";
    where->op = Predicate::Op::Eq;
    where->rhs = ValueFactory::createString("Grace");
    auto st = rs.updateRows("person", assigns, where);
    assert(st.ok());

    auto res = rs.select("person", {"name", "age"}, std::nullopt);
    assert(res.hasValue());
    const ResultSet &s = res.value();
    assert(s.rowCount() == 2);
    // Find Grace row and check age==41
    size_t nameIdx = s.findColumn("name");
    size_t ageIdx = s.findColumn("age");
    bool ok = false;
    for (size_t i = 0; i < s.rowCount(); ++i) {
      if (static_cast<const StringValue &>(s.at(i, nameIdx)).value() ==
          std::string("Grace")) {
        ok = (static_cast<const IntegerValue &>(s.at(i, ageIdx)).value() == 41);
        break;
      }
    }
    assert(ok);
  }

  // updateRows error: unknown column
  {
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["does_not_exist"] = ValueFactory::createInteger(1);
    auto st = rs.updateRows("person", assigns, std::nullopt);
    assert(!st.ok());
    assert(st.code() == StatusCode::InvalidArgument);
  }

  // updateRows error: uniqueness violation (attempt to set duplicate id)
  {
    // Set id=1 for where name=="Grace" which would duplicate id 1
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["id"] = ValueFactory::createInteger(1);
    std::optional<Predicate> where;
    where.emplace();
    where->column = "name";
    where->op = Predicate::Op::Eq;
    where->rhs = ValueFactory::createString("Grace");
    auto st = rs.updateRows("person", assigns, where);
    assert(!st.ok());
    assert(st.code() == StatusCode::FailedPrecondition);
  }

  // deleteRows: delete where age > 40 (should remove Grace with age 41)
  {
    std::optional<Predicate> where;
    where.emplace();
    where->column = "age";
    where->op = Predicate::Op::Gt;
    where->rhs = ValueFactory::createInteger(40);
    auto res = rs.deleteRows("person", where);
    assert(res.hasValue());
    assert(res.value() == static_cast<size_t>(1));

    auto check = rs.select("person", {}, std::nullopt);
    assert(check.hasValue());
    assert(check.value().rowCount() == 1);
  }

  // deleteRows: no matches returns 0
  {
    std::optional<Predicate> where;
    where.emplace();
    where->column = "age";
    where->op = Predicate::Op::Gt;
    where->rhs = ValueFactory::createInteger(1000);
    auto res = rs.deleteRows("person", where);
    assert(res.hasValue());
    assert(res.value() == static_cast<size_t>(0));
  }

  // dropTable: drop and verify NotFound for subsequent operations
  {
    auto st = rs.dropTable("person");
    assert(st.ok());
    auto res = rs.select("person", {}, std::nullopt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::NotFound);
  }

  // Re-create and test: update null into non-nullable column should fail,
  // and truncateTable clears rows but keeps schema
  {
    auto schema2 = makePersonSchema();
    auto stc = rs.createTable("person2", schema2);
    assert(stc.ok());
    Row r(schema2.columns().size());
    r.set(0, ValueFactory::createInteger(10));
    r.set(1, ValueFactory::createString("Bob"));
    r.set(2, ValueFactory::createInteger(20));
    auto sti = rs.insertRow("person2", r);
    assert(sti.ok());

    // Try to set name=null (non-nullable) -> should fail InvalidArgument
    std::unordered_map<std::string, std::unique_ptr<Value>> assigns;
    assigns["name"] = nullptr;
    auto stu = rs.updateRows("person2", assigns, std::nullopt);
    assert(!stu.ok());
    assert(stu.code() == StatusCode::InvalidArgument);

    // truncateTable: delete all rows, keep schema
    auto stt = rs.truncateTable("person2");
    assert(stt.ok());
    auto sel = rs.select("person2", {}, std::nullopt);
    assert(sel.hasValue());
    assert(sel.value().rowCount() == 0);
    // schema intact: insert again should work
    Row r2(schema2.columns().size());
    r2.set(0, ValueFactory::createInteger(11));
    r2.set(1, ValueFactory::createString("Eve"));
    auto sti2 = rs.insertRow("person2", r2);
    assert(sti2.ok());
  }

  // Document tests
  InMemoryDocumentStorage ds;
  {
    Document d;
    d["k"] = ValueFactory::createString("v");
    auto st = ds.put("cfg", "a", d);
    assert(st.ok());
  }
  {
    auto g = ds.get("cfg", "a");
    assert(g.hasValue());
    assert(g.value().at("k")->type() == ValueType::String);
  }
  {
    auto g = ds.get("cfg", "missing");
    assert(!g.hasValue());
  }

  std::cout << "storage_api_test passed\n";
  return 0;
}
