#include <cassert>
#include <memory>
#include <string>

#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

static void test_row_shallow_copy_aliasing() {
  Row r(3);
  r.set(0, ValueFactory::createInteger(42));
  r.set(1, ValueFactory::createString("hello"));
  r.set(2, ValueFactory::createBoolean(true));

  RowShallow rs1 = RowShallow::fromClones(r);
  assert(rs1.size() == r.size());

  // Copying RowShallow should be shallow (shared_ptr aliasing)
  RowShallow rs2 = rs1;
  assert(rs2.size() == rs1.size());
  for (size_t i = 0; i < rs1.size(); ++i) {
    // Values compare equal
    assert(rs1.at(i) == rs2.at(i));
    // And point to the same underlying object
    assert(&rs1.at(i) == &rs2.at(i));
  }

  // Replacing a shared_ptr in rs2 should not affect rs1 values at different
  // indices
  rs2.set(0,
          std::shared_ptr<Value>(ValueFactory::createInteger(100).release()));
  assert(static_cast<const IntegerValue &>(rs2.at(0)).value() == 100);
  // rs1 still sees the old value at index 0 because the pointer was replaced
  // only in rs2
  assert(static_cast<const IntegerValue &>(rs1.at(0)).value() == 42);
}

static void test_row_shallow_to_deep_conversion() {
  Row r(2);
  r.set(0, ValueFactory::createString("abc"));
  r.set(1, ValueFactory::createInteger(7));

  RowShallow rs = RowShallow::fromClones(r);
  Row deep = rs.toRowDeep();

  assert(deep.size() == rs.size());
  for (size_t i = 0; i < rs.size(); ++i) {
    // Equal values
    assert(deep.at(i) == rs.at(i));
    // Different addresses (deep clone)
    assert(&deep.at(i) != &rs.at(i));
  }

  // Mutate deep row; shallow row unaffected
  deep.set(1, ValueFactory::createInteger(99));
  assert(static_cast<const IntegerValue &>(rs.at(1)).value() == 7);
  assert(static_cast<const IntegerValue &>(deep.at(1)).value() == 99);
}

int main() {
  test_row_shallow_copy_aliasing();
  test_row_shallow_to_deep_conversion();
  return 0;
}
