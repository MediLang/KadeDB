#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/serialization.h"
#include "kadedb/value.h"

using namespace kadedb;

struct Stat { double ms_serialize{}; double ms_deserialize{}; };

template <class Fun>
static double time_ms(size_t iters, Fun&& f) {
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < iters; ++i) f();
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

static std::vector<std::unique_ptr<Value>> sampleValues(size_t n) {
  std::vector<std::unique_ptr<Value>> out; out.reserve(n);
  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int> pick(0, 4);
  std::uniform_int_distribution<int> intd(-100000, 100000);
  std::uniform_real_distribution<double> fd(-1000.0, 1000.0);
  for (size_t i = 0; i < n; ++i) {
    switch (pick(rng)) {
      case 0: out.emplace_back(ValueFactory::createNull()); break;
      case 1: out.emplace_back(ValueFactory::createInteger(intd(rng))); break;
      case 2: out.emplace_back(ValueFactory::createFloat(fd(rng))); break;
      case 3: out.emplace_back(ValueFactory::createString("str_" + std::to_string(i))); break;
      default: out.emplace_back(ValueFactory::createBoolean((i & 1) != 0)); break;
    }
  }
  return out;
}

static Row sampleRow() {
  Row r(5);
  r.set(0, ValueFactory::createInteger(123456));
  r.set(1, ValueFactory::createFloat(3.14159));
  r.set(2, ValueFactory::createString("hello world"));
  r.set(3, ValueFactory::createBoolean(true));
  r.set(4, ValueFactory::createNull());
  return r;
}

static TableSchema sampleTableSchema() {
  Column id{"id", ColumnType::Integer, false, true, {}};
  Column name{"name", ColumnType::String, false, false, {}};
  name.constraints.minLength = 1u;
  name.constraints.maxLength = 64u;
  Column age{"age", ColumnType::Integer, true, false, {}};
  age.constraints.minValue = 0.0;
  Column active{"active", ColumnType::Boolean, false, false, {}};
  return TableSchema({id, name, age, active}, std::optional<std::string>("id"));
}

static DocumentSchema sampleDocumentSchema() {
  DocumentSchema ds;
  Column id{"_id", ColumnType::String, false, true, {}};
  id.constraints.minLength = 1u;
  id.constraints.maxLength = 64u;
  Column score{"score", ColumnType::Float, true, false, {}};
  Column flag{"flag", ColumnType::Boolean, false, false, {}};
  ds.addField(id);
  ds.addField(score);
  ds.addField(flag);
  return ds;
}

static Stat bench_value(size_t n) {
  auto vals = sampleValues(n);
  // Binary
  double b_ser = time_ms(n, [&](){
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    for (auto& v : vals) bin::writeValue(*v, ss);
  });
  double b_de = time_ms(n, [&](){
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    for (auto& v : vals) bin::writeValue(*v, ss);
    ss.seekg(0);
    for (size_t i=0;i<vals.size();++i) (void)bin::readValue(ss);
  });
  // JSON
  std::vector<std::string> jsons; jsons.reserve(vals.size());
  double j_ser = time_ms(n, [&](){
    jsons.clear(); jsons.reserve(vals.size());
    for (auto& v : vals) jsons.emplace_back(json::toJson(*v));
  });
  double j_de = time_ms(n, [&](){
    for (auto& s : jsons) (void)json::fromJson(s);
  });
  return {b_ser + 0.0, b_de + 0.0}; // we’ll print JSON separately below
}

int main() {
  const size_t iters = 50; // repetition multiplier for stability
  const size_t N = 1000;   // dataset size per iteration

  std::cout << "Serialization Benchmarks (ms)\n";
  std::cout << "Iterations: " << iters << ", N per iter: " << N << "\n\n";

  // Values
  auto vals = sampleValues(N);
  // Binary serialize
  double v_b_ser = time_ms(iters, [&](){
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    for (auto& v : vals) bin::writeValue(*v, ss);
  });
  // Binary deserialize
  double v_b_de = time_ms(iters, [&](){
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    for (auto& v : vals) bin::writeValue(*v, ss);
    ss.seekg(0);
    for (size_t i=0;i<vals.size();++i) (void)bin::readValue(ss);
  });
  // JSON serialize/deserialize
  std::vector<std::string> v_json; v_json.reserve(vals.size());
  double v_j_ser = time_ms(iters, [&](){
    v_json.clear(); v_json.reserve(vals.size());
    for (auto& v : vals) v_json.emplace_back(json::toJson(*v));
  });
  double v_j_de = time_ms(iters, [&](){
    for (auto& s : v_json) (void)json::fromJson(s);
  });

  std::cout << "Values:\n";
  std::cout << "  Binary   ser: " << v_b_ser << ", de: " << v_b_de << "\n";
  std::cout << "  JSON     ser: " << v_j_ser << ", de: " << v_j_de << "\n\n";

  // Row
  Row r = sampleRow();
  double r_b_ser = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeRow(r, ss); });
  double r_b_de = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeRow(r, ss); ss.seekg(0); (void)bin::readRow(ss); });
  std::string r_json = json::toJson(r);
  double r_j_ser = time_ms(iters * 1000, [&](){ (void)json::toJson(r); });
  double r_j_de = time_ms(iters * 1000, [&](){ (void)json::rowFromJson(r_json); });

  std::cout << "Row:\n";
  std::cout << "  Binary   ser: " << r_b_ser << ", de: " << r_b_de << "\n";
  std::cout << "  JSON     ser: " << r_j_ser << ", de: " << r_j_de << "\n\n";

  // TableSchema
  TableSchema ts = sampleTableSchema();
  double ts_b_ser = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeTableSchema(ts, ss); });
  double ts_b_de = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeTableSchema(ts, ss); ss.seekg(0); (void)bin::readTableSchema(ss); });
  std::string ts_json = json::toJson(ts);
  double ts_j_ser = time_ms(iters * 1000, [&](){ (void)json::toJson(ts); });
  double ts_j_de = time_ms(iters * 1000, [&](){ (void)json::tableSchemaFromJson(ts_json); });

  std::cout << "TableSchema:\n";
  std::cout << "  Binary   ser: " << ts_b_ser << ", de: " << ts_b_de << "\n";
  std::cout << "  JSON     ser: " << ts_j_ser << ", de: " << ts_j_de << "\n\n";

  // DocumentSchema
  DocumentSchema ds = sampleDocumentSchema();
  double ds_b_ser = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeDocumentSchema(ds, ss); });
  double ds_b_de = time_ms(iters * 1000, [&](){ std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary); bin::writeDocumentSchema(ds, ss); ss.seekg(0); (void)bin::readDocumentSchema(ss); });
  std::string ds_json = json::toJson(ds);
  double ds_j_ser = time_ms(iters * 1000, [&](){ (void)json::toJson(ds); });
  double ds_j_de = time_ms(iters * 1000, [&](){ (void)json::documentSchemaFromJson(ds_json); });

  std::cout << "DocumentSchema:\n";
  std::cout << "  Binary   ser: " << ds_b_ser << ", de: " << ds_b_de << "\n";
  std::cout << "  JSON     ser: " << ds_j_ser << ", de: " << ds_j_de << "\n";

  return 0;
}
