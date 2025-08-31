#include <gtest/gtest.h>
#include <sstream>

#include "kadedb/serialization.h"
#include "kadedb/schema.h"
#include "kadedb/value.h"

using namespace kadedb;

TEST(Serialization, ValueRoundTripBinary) {
  std::vector<std::unique_ptr<Value>> vals;
  vals.emplace_back(ValueFactory::createNull());
  vals.emplace_back(ValueFactory::createInteger(42));
  vals.emplace_back(ValueFactory::createFloat(3.14159));
  vals.emplace_back(ValueFactory::createString("hello"));
  vals.emplace_back(ValueFactory::createBoolean(true));

  for (const auto& v : vals) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    // use row wrapper to include header easily
    Row r(1);
    r.set(0, v->clone());
    bin::writeRow(r, ss);
    ss.seekg(0);
    Row r2 = bin::readRow(ss);
    ASSERT_EQ(r2.size(), 1u);
    const Value* v2 = &r2.at(0);
    if (v->type() == ValueType::Null) {
      EXPECT_EQ(v2->type(), ValueType::Null);
    } else {
      EXPECT_TRUE(v->equals(*v2)) << "Expected " << v->toString() << " got " << v2->toString();
    }
  }
}

TEST(Serialization, SchemaRoundTripBinary) {
  TableSchema ts({
      Column{"id", ColumnType::Integer, false, true, {}},
      Column{"name", ColumnType::String, false, false, {.minLength=1u, .maxLength=100u}},
      Column{"age", ColumnType::Integer, true, false, {.minValue=0.0}},
  }, std::optional<std::string>("id"));

  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  bin::writeTableSchema(ts, ss);
  ss.seekg(0);
  TableSchema ts2 = bin::readTableSchema(ss);
  ASSERT_EQ(ts2.columns().size(), ts.columns().size());
  EXPECT_EQ(ts2.primaryKey().value(), "id");
}

TEST(Serialization, ValueRoundTripJSON) {
  auto v = ValueFactory::createString("json");
  auto s = json::toJson(*v);
  auto v2 = json::fromJson(s);
  EXPECT_EQ(v2->type(), ValueType::String);
  EXPECT_TRUE(v->equals(*v2));
}

TEST(Serialization, RowRoundTripJSON) {
  Row r(3);
  r.set(0, ValueFactory::createInteger(7));
  r.set(1, ValueFactory::createNull());
  r.set(2, ValueFactory::createBoolean(false));
  auto s = json::toJson(r);
  Row r2 = json::rowFromJson(s);
  ASSERT_EQ(r2.size(), 3u);
  EXPECT_EQ(r2.at(0).asInt(), 7);
  EXPECT_EQ(r2.at(1).type(), ValueType::Null);
  EXPECT_EQ(static_cast<const BooleanValue&>(r2.at(2)).value(), false);
}
