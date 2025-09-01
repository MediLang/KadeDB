#include <cassert>
#include <string>
#include <unordered_map>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

static void test_row_copy_move() {
  Row r1(3);
  r1.set(0, ValueFactory::createInteger(123));
  r1.set(1, ValueFactory::createString("abc"));
  r1.set(2, ValueFactory::createBoolean(true));

  // Copy construct (deep)
  Row r2 = r1;
  assert(r2.size() == r1.size());
  // Values equal
  for (size_t i = 0; i < r1.size(); ++i) {
    assert(r1.at(i) == r2.at(i));
    // Different addresses -> deep copy
    assert(&r1.at(i) != &r2.at(i));
  }
  // Mutate copy, original unchanged
  r2.set(0, ValueFactory::createInteger(999));
  assert(static_cast<const IntegerValue &>(r1.at(0)).value() == 123);
  assert(static_cast<const IntegerValue &>(r2.at(0)).value() == 999);

  // Copy assign (deep)
  Row r3(0);
  r3 = r1;
  assert(r3.size() == r1.size());
  for (size_t i = 0; i < r1.size(); ++i) {
    assert(r1.at(i) == r3.at(i));
    assert(&r1.at(i) != &r3.at(i));
  }

  // Move construct/assign
  Row r4 = std::move(r3);
  assert(r4.size() == r1.size());

  Row r5(0);
  r5 = std::move(r4);
  assert(r5.size() == r1.size());
}

static void test_document_deep_copy() {
  Document d;
  d.emplace("a", ValueFactory::createInteger(7));
  d.emplace("b", ValueFactory::createString("x"));
  d.emplace("c", ValueFactory::createNull());

  Document d2 = deepCopyDocument(d);
  assert(d2.size() == d.size());
  // Compare equality and address inequality
  for (const auto &kv : d) {
    const auto &key = kv.first;
    const auto &v1 = kv.second;
    const auto &v2 = d2.at(key);
    if (v1 && v2) {
      assert(*v1 == *v2);
      assert(v1.get() != v2.get());
    } else {
      assert(!v1 && !v2);
    }
  }

  // Mutate copy, original unchanged
  d2["a"] = ValueFactory::createInteger(100);
  assert(static_cast<IntegerValue &>(*d["a"]).value() == 7);
  assert(static_cast<IntegerValue &>(*d2["a"]).value() == 100);
}

static void test_schema_copy_move() {
  TableSchema ts(
      {Column{"id", ColumnType::Integer}, Column{"name", ColumnType::String}},
      std::string("id"));
  TableSchema ts2 = ts; // copy
  assert(ts2.columns().size() == ts.columns().size());
  TableSchema ts3 = std::move(ts2); // move
  assert(ts3.columns().size() == ts.columns().size());

  DocumentSchema ds;
  ds.addField(Column{"age", ColumnType::Integer});
  ds.addField(Column{"ok", ColumnType::Boolean});
  DocumentSchema ds2 = ds;
  Column out{};
  assert(ds2.getField("age", out));
  DocumentSchema ds3 = std::move(ds2);
  assert(ds3.getField("ok", out));
}

int main() {
  test_row_copy_move();
  test_document_deep_copy();
  test_schema_copy_move();
  return 0;
}
