#include <cassert>
#include <memory>
#include <string>
#include "kadedb/value.h"

using namespace kadedb;

int main() {
  // Integer
  IntegerValue iv(42);
  assert(iv.type() == ValueType::Integer);
  assert(iv.asInt() == 42);
  assert(iv.toString() == std::string("42"));

  // Float
  FloatValue fv(3.14);
  assert(fv.type() == ValueType::Float);
  assert(fv.asFloat() == 3.14);

  // String
  StringValue sv("hello");
  assert(sv.type() == ValueType::String);
  assert(sv.asString() == std::string("hello"));

  // Boolean
  BooleanValue bv(true);
  assert(bv.type() == ValueType::Boolean);
  assert(bv.asBool() == true);
  assert(bv.asInt() == 1);

  // Null
  NullValue nv;
  assert(nv.type() == ValueType::Null);
  assert(nv.toString() == std::string("null"));

  // Equality / compare
  IntegerValue iv2(42);
  assert(iv == iv2);
  IntegerValue iv3(7);
  assert(iv > iv3);

  // Cross numeric compare
  assert(iv == FloatValue(42.0));
  assert(iv > FloatValue(41.9));

  // String compare
  assert(StringValue("a") < StringValue("b"));

  // Factory
  auto p1 = ValueFactory::createInteger(10);
  auto p2 = ValueFactory::createFloat(10.0);
  assert(*p1 == *p2);

  // Clone
  auto c = p1->clone();
  assert(c->type() == ValueType::Integer);
  assert(static_cast<IntegerValue&>(*c).asInt() == 10);

  return 0;
}
