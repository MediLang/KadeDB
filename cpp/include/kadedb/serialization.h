#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

namespace kadedb {

// Version/tagging for binary format
namespace serialization_constants {
static constexpr uint32_t MAGIC = 0x4B444256; // 'KDBV'
static constexpr uint8_t VERSION = 1;         // bump on format changes
} // namespace serialization_constants

// Error type for (de)serialization problems
class SerializationError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// Binary serialization API
namespace bin {
// Values
void writeValue(const Value &v, std::ostream &os);
std::unique_ptr<Value> readValue(std::istream &is);

// Rows
void writeRow(const Row &row, std::ostream &os);
Row readRow(std::istream &is);

// TableSchema
void writeTableSchema(const TableSchema &schema, std::ostream &os);
TableSchema readTableSchema(std::istream &is);

// DocumentSchema
void writeDocumentSchema(const DocumentSchema &schema, std::ostream &os);
DocumentSchema readDocumentSchema(std::istream &is);

// Document
void writeDocument(const Document &doc, std::ostream &os);
Document readDocument(std::istream &is);
} // namespace bin

// JSON serialization API (text). Produces/consumes strict JSON.
namespace json {
// Values
std::string toJson(const Value &v);
std::unique_ptr<Value> fromJson(const std::string &json);

// Row
std::string toJson(const Row &row);
Row rowFromJson(const std::string &json);

// TableSchema
std::string toJson(const TableSchema &schema);
TableSchema tableSchemaFromJson(const std::string &json);

// DocumentSchema
std::string toJson(const DocumentSchema &schema);
DocumentSchema documentSchemaFromJson(const std::string &json);

// Document
std::string toJson(const Document &doc);
Document documentFromJson(const std::string &json);
} // namespace json

} // namespace kadedb
