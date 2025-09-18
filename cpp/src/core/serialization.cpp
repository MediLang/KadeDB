#include "kadedb/serialization.h"

#include <cstring>
#include <sstream>

namespace kadedb {
namespace {

// Helpers for binary IO with error checks
inline void writeU8(std::ostream &os, uint8_t v) {
  os.write(reinterpret_cast<const char *>(&v), 1);
}
inline void writeU32(std::ostream &os, uint32_t v) {
  os.write(reinterpret_cast<const char *>(&v), 4);
}
inline void writeI64(std::ostream &os, int64_t v) {
  os.write(reinterpret_cast<const char *>(&v), 8);
}
inline void writeF64(std::ostream &os, double v) {
  os.write(reinterpret_cast<const char *>(&v), 8);
}

inline uint8_t readU8(std::istream &is) {
  uint8_t v{};
  if (!is.read(reinterpret_cast<char *>(&v), 1))
    throw SerializationError("Unexpected EOF reading u8");
  return v;
}
inline uint32_t readU32(std::istream &is) {
  uint32_t v{};
  if (!is.read(reinterpret_cast<char *>(&v), 4))
    throw SerializationError("Unexpected EOF reading u32");
  return v;
}
inline int64_t readI64(std::istream &is) {
  int64_t v{};
  if (!is.read(reinterpret_cast<char *>(&v), 8))
    throw SerializationError("Unexpected EOF reading i64");
  return v;
}
inline double readF64(std::istream &is) {
  double v{};
  if (!is.read(reinterpret_cast<char *>(&v), 8))
    throw SerializationError("Unexpected EOF reading f64");
  return v;
}

inline void writeString(std::ostream &os, const std::string &s) {
  writeU32(os, static_cast<uint32_t>(s.size()));
  if (!s.empty())
    os.write(s.data(), static_cast<std::streamsize>(s.size()));
}
inline std::string readString(std::istream &is) {
  uint32_t n = readU32(is);
  std::string s(n, '\0');
  if (n && !is.read(&s[0], n))
    throw SerializationError("Unexpected EOF reading string");
  return s;
}

inline void writeHeader(std::ostream &os) {
  writeU32(os, serialization_constants::MAGIC);
  writeU8(os, serialization_constants::VERSION);
}
inline void readHeader(std::istream &is) {
  uint32_t magic = readU32(is);
  if (magic != serialization_constants::MAGIC)
    throw SerializationError("Bad magic");
  uint8_t version = readU8(is);
  if (version != serialization_constants::VERSION)
    throw SerializationError("Unsupported version");
}

// JSON helpers (very small, not a full JSON implementation). We emit simple
// compact JSON
static inline std::string jsonEscape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      if (static_cast<unsigned char>(c) < 0x20) {
        // control char -> skip
      } else
        out += c;
    }
  }
  return out;
}

} // namespace

namespace bin {

void writeValue(const Value &v, std::ostream &os) {
  // No header here; callers at top-level should write header once
  writeU8(os, static_cast<uint8_t>(v.type()));
  switch (v.type()) {
  case ValueType::Null:
    break;
  case ValueType::Integer:
    writeI64(os, static_cast<const IntegerValue &>(v).value());
    break;
  case ValueType::Float:
    writeF64(os, static_cast<const FloatValue &>(v).value());
    break;
  case ValueType::String:
    writeString(os, static_cast<const StringValue &>(v).value());
    break;
  case ValueType::Boolean:
    writeU8(os,
            static_cast<uint8_t>(static_cast<const BooleanValue &>(v).value()));
    break;
  }
}

std::unique_ptr<Value> readValue(std::istream &is) {
  auto t = static_cast<ValueType>(readU8(is));
  switch (t) {
  case ValueType::Null:
    return ValueFactory::createNull();
  case ValueType::Integer:
    return ValueFactory::createInteger(readI64(is));
  case ValueType::Float:
    return ValueFactory::createFloat(readF64(is));
  case ValueType::String:
    return ValueFactory::createString(readString(is));
  case ValueType::Boolean:
    return ValueFactory::createBoolean(readU8(is) != 0);
  }
  throw SerializationError("Unknown ValueType");
}

void writeRow(const Row &row, std::ostream &os) {
  writeHeader(os);
  writeU32(os, static_cast<uint32_t>(row.size()));
  for (size_t i = 0; i < row.size(); ++i) {
    const auto &ptr = row.values()[i];
    uint8_t isNull = ptr ? 0 : 1;
    writeU8(os, isNull);
    if (!isNull)
      writeValue(*ptr, os);
  }
}

Row readRow(std::istream &is) {
  readHeader(is);
  uint32_t n = readU32(is);
  Row row(n);
  for (uint32_t i = 0; i < n; ++i) {
    uint8_t isNull = readU8(is);
    if (!isNull)
      row.set(i, readValue(is));
  }
  return row;
}

void writeTableSchema(const TableSchema &schema, std::ostream &os) {
  writeHeader(os);
  // columns
  const auto &cols = schema.columns();
  writeU32(os, static_cast<uint32_t>(cols.size()));
  for (const auto &c : cols) {
    writeString(os, c.name);
    writeU8(os, static_cast<uint8_t>(c.type));
    writeU8(os, c.nullable ? 1 : 0);
    writeU8(os, c.unique ? 1 : 0);
    // constraints
    // strings
    writeU8(os, c.constraints.minLength.has_value());
    if (c.constraints.minLength)
      writeU32(os, static_cast<uint32_t>(*c.constraints.minLength));
    writeU8(os, c.constraints.maxLength.has_value());
    if (c.constraints.maxLength)
      writeU32(os, static_cast<uint32_t>(*c.constraints.maxLength));
    writeU32(os, static_cast<uint32_t>(c.constraints.oneOf.size()));
    for (const auto &s : c.constraints.oneOf)
      writeString(os, s);
    // numeric
    writeU8(os, c.constraints.minValue.has_value());
    if (c.constraints.minValue)
      writeF64(os, *c.constraints.minValue);
    writeU8(os, c.constraints.maxValue.has_value());
    if (c.constraints.maxValue)
      writeF64(os, *c.constraints.maxValue);
  }
  // primary key
  const auto &pk = schema.primaryKey();
  writeU8(os, pk.has_value());
  if (pk)
    writeString(os, *pk);
}

TableSchema readTableSchema(std::istream &is) {
  readHeader(is);
  uint32_t ncols = readU32(is);
  std::vector<Column> cols;
  cols.reserve(ncols);
  for (uint32_t i = 0; i < ncols; ++i) {
    Column c{};
    c.name = readString(is);
    c.type = static_cast<ColumnType>(readU8(is));
    c.nullable = readU8(is) != 0;
    c.unique = readU8(is) != 0;
    // constraints
    if (readU8(is))
      c.constraints.minLength = readU32(is);
    if (readU8(is))
      c.constraints.maxLength = readU32(is);
    uint32_t oneOfN = readU32(is);
    c.constraints.oneOf.clear();
    c.constraints.oneOf.reserve(oneOfN);
    for (uint32_t k = 0; k < oneOfN; ++k)
      c.constraints.oneOf.emplace_back(readString(is));
    if (readU8(is))
      c.constraints.minValue = readF64(is);
    if (readU8(is))
      c.constraints.maxValue = readF64(is);
    cols.push_back(std::move(c));
  }
  std::optional<std::string> pk;
  if (readU8(is))
    pk = readString(is);
  return TableSchema(std::move(cols), std::move(pk));
}

void writeDocumentSchema(const DocumentSchema &schema, std::ostream &os) {
  writeHeader(os);
  const auto &fields = schema.fields();
  writeU32(os, static_cast<uint32_t>(fields.size()));
  for (const auto &kv : fields) {
    const auto &name = kv.first;
    const auto &c = kv.second;
    writeString(os, name);
    writeU8(os, static_cast<uint8_t>(c.type));
    writeU8(os, c.nullable ? 1 : 0);
    writeU8(os, c.unique ? 1 : 0);
    if (c.constraints.minLength) {
      writeU8(os, 1);
      writeU32(os, static_cast<uint32_t>(*c.constraints.minLength));
    } else
      writeU8(os, 0);
    if (c.constraints.maxLength) {
      writeU8(os, 1);
      writeU32(os, static_cast<uint32_t>(*c.constraints.maxLength));
    } else
      writeU8(os, 0);
    writeU32(os, static_cast<uint32_t>(c.constraints.oneOf.size()));
    for (const auto &s : c.constraints.oneOf)
      writeString(os, s);
    if (c.constraints.minValue) {
      writeU8(os, 1);
      writeF64(os, *c.constraints.minValue);
    } else
      writeU8(os, 0);
    if (c.constraints.maxValue) {
      writeU8(os, 1);
      writeF64(os, *c.constraints.maxValue);
    } else
      writeU8(os, 0);
  }
}

DocumentSchema readDocumentSchema(std::istream &is) {
  readHeader(is);
  uint32_t n = readU32(is);
  DocumentSchema ds;
  for (uint32_t i = 0; i < n; ++i) {
    Column c{};
    std::string name = readString(is);
    c.name = name;
    c.type = static_cast<ColumnType>(readU8(is));
    c.nullable = readU8(is) != 0;
    c.unique = readU8(is) != 0;
    if (readU8(is))
      c.constraints.minLength = readU32(is);
    if (readU8(is))
      c.constraints.maxLength = readU32(is);
    uint32_t oneOfN = readU32(is);
    for (uint32_t k = 0; k < oneOfN; ++k)
      c.constraints.oneOf.emplace_back(readString(is));
    if (readU8(is))
      c.constraints.minValue = readF64(is);
    if (readU8(is))
      c.constraints.maxValue = readF64(is);
    ds.addField(std::move(c));
  }
  return ds;
}

void writeDocument(const Document &doc, std::ostream &os) {
  writeHeader(os);
  writeU32(os, static_cast<uint32_t>(doc.size()));
  for (const auto &kv : doc) {
    writeString(os, kv.first);
    uint8_t isNull = kv.second ? 0 : 1;
    writeU8(os, isNull);
    if (!isNull)
      writeValue(*kv.second, os);
  }
}

Document readDocument(std::istream &is) {
  readHeader(is);
  uint32_t n = readU32(is);
  Document d;
  for (uint32_t i = 0; i < n; ++i) {
    std::string name = readString(is);
    uint8_t isNull = readU8(is);
    if (!isNull) {
      d.try_emplace(std::move(name), readValue(is));
    } else {
      d.try_emplace(std::move(name), nullptr);
    }
  }
  return d;
}

} // namespace bin

namespace json {

// Value JSON: {"t":"null|int|float|string|bool","v":...}
static inline const char *typeToStr(ValueType t) {
  switch (t) {
  case ValueType::Null:
    return "null";
  case ValueType::Integer:
    return "int";
  case ValueType::Float:
    return "float";
  case ValueType::String:
    return "string";
  case ValueType::Boolean:
    return "bool";
  }
  return "unknown";
}

std::string toJson(const Value &v) {
  std::ostringstream oss;
  oss << "{\"t\":\"" << typeToStr(v.type()) << "\",";
  oss << "\"v\":";
  switch (v.type()) {
  case ValueType::Null:
    oss << "null";
    break;
  case ValueType::Integer:
    oss << static_cast<const IntegerValue &>(v).value();
    break;
  case ValueType::Float:
    oss << static_cast<const FloatValue &>(v).toString();
    break;
  case ValueType::String:
    oss << '"' << jsonEscape(static_cast<const StringValue &>(v).value())
        << '"';
    break;
  case ValueType::Boolean:
    oss << (static_cast<const BooleanValue &>(v).value() ? "true" : "false");
    break;
  }
  oss << '}';
  return oss.str();
}

std::unique_ptr<Value> fromJson(const std::string &jsonStr) {
  // Very small parser assuming our own format
  auto tpos = jsonStr.find("\"t\"");
  auto vpos = jsonStr.find("\"v\"");
  if (tpos == std::string::npos || vpos == std::string::npos)
    throw SerializationError("Bad JSON Value");
  auto tstart = jsonStr.find('"', tpos + 3);
  if (tstart == std::string::npos)
    throw SerializationError("Bad JSON");
  auto tend = jsonStr.find('"', tstart + 1);
  if (tend == std::string::npos)
    throw SerializationError("Bad JSON");
  std::string t = jsonStr.substr(tstart + 1, tend - tstart - 1);
  auto vstart = jsonStr.find(':', vpos);
  if (vstart == std::string::npos)
    throw SerializationError("Bad JSON");
  ++vstart;
  // trim spaces
  while (vstart < jsonStr.size() &&
         isspace(static_cast<unsigned char>(jsonStr[vstart])))
    ++vstart;
  std::string v = jsonStr.substr(vstart);
  // remove trailing }
  if (!v.empty() && v.back() == '}')
    v.pop_back();
  while (!v.empty() && isspace(static_cast<unsigned char>(v.back())))
    v.pop_back();
  if (t == "null")
    return ValueFactory::createNull();
  if (t == "int")
    return ValueFactory::createInteger(std::stoll(v));
  if (t == "float")
    return ValueFactory::createFloat(std::stod(v));
  if (t == "string") {
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
      return ValueFactory::createString(v.substr(1, v.size() - 2));
    throw SerializationError("Bad JSON string value");
  }
  if (t == "bool") {
    if (v.rfind("true", 0) == 0)
      return ValueFactory::createBoolean(true);
    if (v.rfind("false", 0) == 0)
      return ValueFactory::createBoolean(false);
    throw SerializationError("Bad JSON bool value");
  }
  throw SerializationError("Unknown Value JSON type");
}

std::string toJson(const Row &row) {
  std::ostringstream oss;
  oss << "{\"values\":[";
  for (size_t i = 0; i < row.size(); ++i) {
    if (i)
      oss << ',';
    const auto &ptr = row.values()[i];
    if (!ptr)
      oss << "null";
    else
      oss << toJson(*ptr);
  }
  oss << "],\"version\":" << static_cast<int>(serialization_constants::VERSION)
      << "}";
  return oss.str();
}

Row rowFromJson(const std::string &s) {
  auto arrPos = s.find("["),
       arrEnd = s.find("]", arrPos == std::string::npos ? 0 : arrPos + 1);
  if (arrPos == std::string::npos || arrEnd == std::string::npos)
    throw SerializationError("Bad Row JSON");
  std::string arr = s.substr(arrPos + 1, arrEnd - arrPos - 1);
  // naive split on commas not inside braces
  std::vector<std::string> items;
  int depth = 0;
  size_t last = 0;
  for (size_t i = 0; i < arr.size(); ++i) {
    char c = arr[i];
    if (c == '{')
      ++depth;
    else if (c == '}')
      --depth;
    else if (c == ',' && depth == 0) {
      items.emplace_back(arr.substr(last, i - last));
      last = i + 1;
    }
  }
  if (last < arr.size())
    items.emplace_back(arr.substr(last));
  Row row(items.size());
  for (size_t i = 0; i < items.size(); ++i) {
    std::string it = items[i];
    // trim
    size_t a = 0, b = it.size();
    while (a < b && isspace(static_cast<unsigned char>(it[a])))
      ++a;
    while (b > a && isspace(static_cast<unsigned char>(it[b - 1])))
      --b;
    if (a == b || (b - a == 4 && it.substr(a, 4) == "null"))
      continue;
    row.set(i, fromJson(it.substr(a, b - a)));
  }
  return row;
}

static inline std::string
constraintsToJson(const Column::ColumnConstraints &cc) {
  std::ostringstream oss;
  oss << "\"minLength\":";
  if (cc.minLength)
    oss << *cc.minLength;
  else
    oss << "null";
  oss << ',';
  oss << "\"maxLength\":";
  if (cc.maxLength)
    oss << *cc.maxLength;
  else
    oss << "null";
  oss << ',';
  oss << "\"oneOf\":[";
  for (size_t i = 0; i < cc.oneOf.size(); ++i) {
    if (i)
      oss << ',';
    oss << '"' << jsonEscape(cc.oneOf[i]) << '"';
  }
  oss << "],";
  oss << "\"minValue\":";
  if (cc.minValue)
    oss << *cc.minValue;
  else
    oss << "null";
  oss << ',';
  oss << "\"maxValue\":";
  if (cc.maxValue)
    oss << *cc.maxValue;
  else
    oss << "null";
  return oss.str();
}

static inline const char *colTypeToStr(ColumnType t) {
  switch (t) {
  case ColumnType::Null:
    return "null";
  case ColumnType::Integer:
    return "integer";
  case ColumnType::Float:
    return "float";
  case ColumnType::String:
    return "string";
  case ColumnType::Boolean:
    return "boolean";
  }
  return "unknown";
}

std::string toJson(const TableSchema &schema) {
  std::ostringstream oss;
  oss << "{\"columns\":[";
  const auto &cols = schema.columns();
  for (size_t i = 0; i < cols.size(); ++i) {
    const auto &c = cols[i];
    if (i)
      oss << ',';
    oss << "{\"name\":\"" << jsonEscape(c.name) << "\",";
    oss << "\"type\":\"" << colTypeToStr(c.type) << "\",";
    oss << "\"nullable\":" << (c.nullable ? "true" : "false") << ",";
    oss << "\"unique\":" << (c.unique ? "true" : "false") << ",";
    oss << "\"constraints\":{" << constraintsToJson(c.constraints) << "}}";
  }
  oss << "],\"primaryKey\":";
  if (schema.primaryKey())
    oss << '"' << jsonEscape(*schema.primaryKey()) << '"';
  else
    oss << "null";
  oss << ",\"version\":" << static_cast<int>(serialization_constants::VERSION)
      << '}';
  return oss.str();
}

// Minimal parser for our schema JSON — expects fields we emit
TableSchema tableSchemaFromJson(const std::string &s) {
  // parse columns
  auto cpos = s.find("\"columns\"");
  if (cpos == std::string::npos)
    throw SerializationError("No columns");
  auto arrPos = s.find('[', cpos);
  auto arrEnd = s.find(']', arrPos == std::string::npos ? 0 : arrPos + 1);
  if (arrPos == std::string::npos || arrEnd == std::string::npos)
    throw SerializationError("Bad columns array");
  std::string arr = s.substr(arrPos + 1, arrEnd - arrPos - 1);
  // split objects
  std::vector<std::string> items;
  int depth = 0;
  size_t last = 0;
  for (size_t i = 0; i < arr.size(); ++i) {
    char c = arr[i];
    if (c == '{')
      ++depth;
    else if (c == '}')
      --depth;
    else if (c == ',' && depth == 0) {
      items.emplace_back(arr.substr(last, i - last));
      last = i + 1;
    }
  }
  if (last < arr.size())
    items.emplace_back(arr.substr(last));
  auto parseType = [](const std::string &t) -> ColumnType {
    if (t == "\"null\"")
      return ColumnType::Null;
    if (t == "\"integer\"")
      return ColumnType::Integer;
    if (t == "\"float\"")
      return ColumnType::Float;
    if (t == "\"string\"")
      return ColumnType::String;
    if (t == "\"boolean\"")
      return ColumnType::Boolean;
    throw SerializationError("Unknown ColumnType");
  };
  std::vector<Column> cols;
  cols.reserve(items.size());
  for (auto it : items) {
    if (it.find('{') == std::string::npos)
      continue;
    Column c{};
    auto npos = it.find("\"name\"");
    auto nstart = it.find('"', npos + 6);
    auto nend = it.find('"', nstart + 1);
    if (npos == std::string::npos || nstart == std::string::npos ||
        nend == std::string::npos)
      throw SerializationError("Bad column name");
    c.name = it.substr(nstart + 1, nend - nstart - 1);
    auto tpos = it.find("\"type\"");
    auto tstart = it.find('"', tpos + 6);
    auto tend = it.find('"', tstart + 1);
    if (tpos == std::string::npos || tstart == std::string::npos ||
        tend == std::string::npos)
      throw SerializationError("Bad column type");
    c.type = parseType(it.substr(tstart, tend - tstart + 1));
    auto nb = it.find("\"nullable\"");
    if (nb == std::string::npos)
      throw SerializationError("Bad nullable");
    c.nullable = it.find("true", nb) != std::string::npos;
    auto ub = it.find("\"unique\"");
    if (ub == std::string::npos)
      throw SerializationError("Bad unique");
    c.unique = it.find("true", ub) != std::string::npos;
    // constraints optional numbers/arrays — keep default if not found
    cols.push_back(std::move(c));
  }
  // primaryKey
  std::optional<std::string> pk;
  auto pkpos = s.find("\"primaryKey\"");
  if (pkpos != std::string::npos) {
    auto q1 = s.find('"', pkpos + 12);
    if (q1 != std::string::npos) {
      auto q2 = s.find('"', q1 + 1);
      if (q2 != std::string::npos)
        pk = s.substr(q1 + 1, q2 - q1 - 1);
    }
  }
  return TableSchema(std::move(cols), std::move(pk));
}

std::string toJson(const DocumentSchema &schema) {
  std::ostringstream oss;
  oss << "{\"fields\":{";
  bool first = true;
  for (const auto &kv : schema.fields()) {
    if (!first) {
      oss << ',';
    }
    first = false;
    const auto &name = kv.first;
    const auto &c = kv.second;
    oss << '"' << jsonEscape(name) << "\":{";
    oss << "\"type\":\"" << colTypeToStr(c.type) << "\",";
    oss << "\"nullable\":" << (c.nullable ? "true" : "false") << ",";
    oss << "\"unique\":" << (c.unique ? "true" : "false") << ",";
    oss << "\"constraints\":{" << constraintsToJson(c.constraints) << "}}";
  }
  oss << "},\"version\":" << static_cast<int>(serialization_constants::VERSION)
      << '}';
  return oss.str();
}

DocumentSchema documentSchemaFromJson(const std::string &s) {
  auto fpos = s.find("\"fields\"");
  if (fpos == std::string::npos)
    throw SerializationError("No fields");
  auto objStart = s.find('{', fpos);
  auto objEnd = s.rfind('}');
  if (objStart == std::string::npos || objEnd == std::string::npos ||
      objEnd <= objStart)
    throw SerializationError("Bad fields object");
  std::string obj = s.substr(objStart + 1, objEnd - objStart - 1);
  DocumentSchema ds;
  // split on commas not inside braces
  int depth = 0;
  size_t last = 0;
  std::vector<std::string> pairs;
  for (size_t i = 0; i < obj.size(); ++i) {
    char c = obj[i];
    if (c == '{')
      ++depth;
    else if (c == '}')
      --depth;
    else if (c == ',' && depth == 0) {
      pairs.emplace_back(obj.substr(last, i - last));
      last = i + 1;
    }
  }
  if (last < obj.size())
    pairs.emplace_back(obj.substr(last));
  for (auto &p : pairs) {
    auto k1 = p.find('"');
    if (k1 == std::string::npos)
      continue;
    auto k2 = p.find('"', k1 + 1);
    if (k2 == std::string::npos)
      continue;
    std::string name = p.substr(k1 + 1, k2 - k1 - 1);
    Column c{};
    c.name = name;
    auto tpos = p.find("\"type\"");
    auto tstart = p.find('"', tpos + 6);
    auto tend = p.find('"', tstart + 1);
    if (tpos == std::string::npos || tstart == std::string::npos ||
        tend == std::string::npos)
      throw SerializationError("Bad type in field");
    std::string ts = p.substr(tstart + 1, tend - tstart - 1);
    if (ts == "integer")
      c.type = ColumnType::Integer;
    else if (ts == "float")
      c.type = ColumnType::Float;
    else if (ts == "string")
      c.type = ColumnType::String;
    else if (ts == "boolean")
      c.type = ColumnType::Boolean;
    else
      c.type = ColumnType::Null;
    c.nullable = p.find("\"nullable\":true") != std::string::npos;
    c.unique = p.find("\"unique\":true") != std::string::npos;
    ds.addField(std::move(c));
  }
  return ds;
}

std::string toJson(const Document &doc) {
  std::ostringstream oss;
  oss << '{';
  bool first = true;
  for (const auto &kv : doc) {
    if (!first)
      oss << ',';
    first = false;
    oss << '"' << jsonEscape(kv.first) << '"' << ':';
    if (!kv.second) {
      oss << "null";
    } else {
      oss << toJson(*kv.second);
    }
  }
  oss << '}';
  return oss.str();
}

Document documentFromJson(const std::string &s) {
  // Expect an object with keys -> Value JSON or null
  size_t objStart = s.find('{');
  size_t objEnd = s.rfind('}');
  if (objStart == std::string::npos || objEnd == std::string::npos ||
      objEnd <= objStart)
    throw SerializationError("Bad Document JSON");
  std::string obj = s.substr(objStart + 1, objEnd - objStart - 1);
  Document d;
  int depth = 0;
  size_t last = 0;
  for (size_t i = 0; i < obj.size(); ++i) {
    char c = obj[i];
    if (c == '{')
      ++depth;
    else if (c == '}')
      --depth;
    else if (c == ',' && depth == 0) {
      std::string pair = obj.substr(last, i - last);
      last = i + 1;
      // parse pair key:value
      size_t q1 = pair.find('"');
      if (q1 == std::string::npos)
        continue;
      size_t q2 = pair.find('"', q1 + 1);
      if (q2 == std::string::npos)
        continue;
      std::string key = pair.substr(q1 + 1, q2 - q1 - 1);
      size_t colon = pair.find(':', q2 + 1);
      if (colon == std::string::npos)
        continue;
      std::string val = pair.substr(colon + 1);
      // trim
      size_t a = 0, b = val.size();
      while (a < b && isspace(static_cast<unsigned char>(val[a])))
        ++a;
      while (b > a && isspace(static_cast<unsigned char>(val[b - 1])))
        --b;
      if (a == b || (b - a == 4 && val.substr(a, 4) == "null")) {
        d.try_emplace(std::move(key), nullptr);
      } else {
        d.try_emplace(std::move(key), fromJson(val.substr(a, b - a)));
      }
    }
  }
  // last segment
  if (last < obj.size()) {
    std::string pair = obj.substr(last);
    size_t q1 = pair.find('"');
    if (q1 != std::string::npos) {
      size_t q2 = pair.find('"', q1 + 1);
      if (q2 != std::string::npos) {
        std::string key = pair.substr(q1 + 1, q2 - q1 - 1);
        size_t colon = pair.find(':', q2 + 1);
        if (colon != std::string::npos) {
          std::string val = pair.substr(colon + 1);
          size_t a = 0, b = val.size();
          while (a < b && isspace(static_cast<unsigned char>(val[a])))
            ++a;
          while (b > a && isspace(static_cast<unsigned char>(val[b - 1])))
            --b;
          if (a == b || (b - a == 4 && val.substr(a, 4) == "null")) {
            d.try_emplace(std::move(key), nullptr);
          } else {
            d.try_emplace(std::move(key), fromJson(val.substr(a, b - a)));
          }
        }
      }
    }
  }
  return d;
}

} // namespace json

} // namespace kadedb
