#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "kadedb/result.h"
#include "kadedb/value.h"

using namespace kadedb;

int main() {
  // Build a small result set
  ResultSet rs({"id", "name", "active"},
               {ColumnType::Integer, ColumnType::String, ColumnType::Boolean});
  {
    std::vector<std::unique_ptr<Value>> vals;
    vals.emplace_back(std::make_unique<IntegerValue>(1));
    vals.emplace_back(std::make_unique<StringValue>("alice"));
    vals.emplace_back(std::make_unique<BooleanValue>(true));
    rs.addRow(ResultRow(std::move(vals)));
  }
  {
    std::vector<std::unique_ptr<Value>> vals;
    vals.emplace_back(std::make_unique<IntegerValue>(2));
    vals.emplace_back(std::make_unique<StringValue>("bob, the \"builder\""));
    vals.emplace_back(std::make_unique<BooleanValue>(false));
    rs.addRow(ResultRow(std::move(vals)));
  }

  // CSV with header
  std::string csv = rs.toCSV(',');
  // Expect header + 2 rows
  assert(csv.find("id,name,active\n") == 0);
  // Ensure escaping for quotes and comma
  assert(csv.find("2,\"bob, the \"\"builder\"\"\",false\n") !=
         std::string::npos);

  // JSON without metadata
  std::string json = rs.toJSON(false);
  // Expect array of 2 objects
  assert(json.size() > 0);
  assert(json.front() == '[');
  assert(json.back() == ']');
  assert(json.find("\"name\":\"alice\"") != std::string::npos);

  // Pagination
  rs.setPageSize(1);
  assert(rs.totalPages() == 2);
  auto p0 = rs.page(0);
  auto p1 = rs.page(1);
  assert(p0.size() == 1 && p1.size() == 1);
  assert(static_cast<const IntegerValue &>(p0[0]->at(0)).asInt() == 1);
  assert(static_cast<const IntegerValue &>(p1[0]->at(0)).asInt() == 2);

  return 0;
}
