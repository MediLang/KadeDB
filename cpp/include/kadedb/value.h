#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace kadedb {

// Identify supported value types
enum class ValueType {
  Null = 0,
  Integer = 1,
  Float = 2,
  String = 3,
  Boolean = 4,
};

// Base Value interface
class Value {
public:
  virtual ~Value() = default;

  // RTTI-like identification
  virtual ValueType type() const = 0;

  // String representation
  virtual std::string toString() const = 0;

  // Polymorphic copy
  virtual std::unique_ptr<Value> clone() const = 0;

  // Conversions (throw by default if not convertible)
  virtual int64_t asInt() const { throw std::runtime_error("Value not convertible to int"); }
  virtual double asFloat() const { throw std::runtime_error("Value not convertible to float"); }
  virtual bool asBool() const { throw std::runtime_error("Value not convertible to bool"); }
  virtual const std::string& asString() const { throw std::runtime_error("Value not convertible to string"); }

  // Equality and ordering
  virtual bool equals(const Value& other) const = 0;
  // Returns negative if *this < other, 0 if equal, positive if *this > other
  virtual int compare(const Value& other) const = 0;

  // Operators leverage virtual functions above
  bool operator==(const Value& other) const { return equals(other); }
  bool operator!=(const Value& other) const { return !equals(other); }
  bool operator<(const Value& other) const { return compare(other) < 0; }
  bool operator>(const Value& other) const { return compare(other) > 0; }
  bool operator<=(const Value& other) const { return compare(other) <= 0; }
  bool operator>=(const Value& other) const { return compare(other) >= 0; }
};

class NullValue final : public Value {
public:
  ValueType type() const override { return ValueType::Null; }
  std::string toString() const override { return "null"; }
  std::unique_ptr<Value> clone() const override { return std::make_unique<NullValue>(); }

  bool equals(const Value& other) const override { return other.type() == ValueType::Null; }
  int compare(const Value& other) const override {
    if (other.type() == ValueType::Null) return 0;
    // Define Null < any non-null
    return -1;
  }
};

class IntegerValue final : public Value {
public:
  explicit IntegerValue(int64_t v) : value_(v) {}

  ValueType type() const override { return ValueType::Integer; }
  std::string toString() const override { return std::to_string(value_); }
  std::unique_ptr<Value> clone() const override { return std::make_unique<IntegerValue>(value_); }

  int64_t asInt() const override { return value_; }
  double asFloat() const override { return static_cast<double>(value_); }
  bool asBool() const override { return value_ != 0; }

  bool equals(const Value& other) const override;
  int compare(const Value& other) const override;

  int64_t value() const { return value_; }

private:
  int64_t value_ = 0;
};

class FloatValue final : public Value {
public:
  explicit FloatValue(double v) : value_(v) {}

  ValueType type() const override { return ValueType::Float; }
  std::string toString() const override;
  std::unique_ptr<Value> clone() const override { return std::make_unique<FloatValue>(value_); }

  double asFloat() const override { return value_; }
  bool asBool() const override { return value_ != 0.0; }

  bool equals(const Value& other) const override;
  int compare(const Value& other) const override;

  double value() const { return value_; }

private:
  double value_ = 0.0;
};

class StringValue final : public Value {
public:
  explicit StringValue(std::string v) : value_(std::move(v)) {}

  ValueType type() const override { return ValueType::String; }
  std::string toString() const override { return value_; }
  std::unique_ptr<Value> clone() const override { return std::make_unique<StringValue>(value_); }

  const std::string& asString() const override { return value_; }
  bool asBool() const override { return !value_.empty(); }

  bool equals(const Value& other) const override;
  int compare(const Value& other) const override;

  const std::string& value() const { return value_; }

private:
  std::string value_;
};

class BooleanValue final : public Value {
public:
  explicit BooleanValue(bool v) : value_(v) {}

  ValueType type() const override { return ValueType::Boolean; }
  std::string toString() const override { return value_ ? "true" : "false"; }
  std::unique_ptr<Value> clone() const override { return std::make_unique<BooleanValue>(value_); }

  bool asBool() const override { return value_; }
  int64_t asInt() const override { return value_ ? 1 : 0; }
  double asFloat() const override { return value_ ? 1.0 : 0.0; }

  bool equals(const Value& other) const override;
  int compare(const Value& other) const override;

  bool value() const { return value_; }

private:
  bool value_ = false;
};

// Factory utilities
struct ValueFactory {
  static std::unique_ptr<Value> createNull();
  static std::unique_ptr<Value> createInteger(int64_t v);
  static std::unique_ptr<Value> createFloat(double v);
  static std::unique_ptr<Value> createString(std::string v);
  static std::unique_ptr<Value> createBoolean(bool v);
};

// Helper to attempt numeric cross-type comparison
inline int compareNumeric(double a, double b) {
  if (a < b) return -1;
  if (a > b) return 1;
  return 0;
}

} // namespace kadedb
