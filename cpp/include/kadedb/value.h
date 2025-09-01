#pragma once

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace kadedb {

// Optional memory debug and pooling
// Define KADEDB_MEM_DEBUG to enable allocation counters per Value type
// Define KADEDB_ENABLE_SMALL_OBJECT_POOL to enable a tiny freelist pool for
// fixed-size Value types (IntegerValue, BooleanValue, NullValue).

namespace memdebug {
// Accessors return cumulative counts since process start
size_t allocCountInteger();
size_t freeCountInteger();
size_t allocCountBoolean();
size_t freeCountBoolean();
size_t allocCountNull();
size_t freeCountNull();
} // namespace memdebug

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
  virtual int64_t asInt() const {
    throw std::runtime_error("Value not convertible to int");
  }
  virtual double asFloat() const {
    throw std::runtime_error("Value not convertible to float");
  }
  virtual bool asBool() const {
    throw std::runtime_error("Value not convertible to bool");
  }
  virtual const std::string &asString() const {
    throw std::runtime_error("Value not convertible to string");
  }

  // Equality and ordering
  virtual bool equals(const Value &other) const = 0;
  // Returns negative if *this < other, 0 if equal, positive if *this > other
  virtual int compare(const Value &other) const = 0;

  // Operators leverage virtual functions above
  bool operator==(const Value &other) const { return equals(other); }
  bool operator!=(const Value &other) const { return !equals(other); }
  bool operator<(const Value &other) const { return compare(other) < 0; }
  bool operator>(const Value &other) const { return compare(other) > 0; }
  bool operator<=(const Value &other) const { return compare(other) <= 0; }
  bool operator>=(const Value &other) const { return compare(other) >= 0; }
};

class NullValue final : public Value {
public:
  ValueType type() const override { return ValueType::Null; }
  std::string toString() const override { return "null"; }
  std::unique_ptr<Value> clone() const override {
    return std::make_unique<NullValue>();
  }

#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
  static void *operator new(std::size_t sz);
  static void operator delete(void *p) noexcept;
#endif

  bool equals(const Value &other) const override {
    return other.type() == ValueType::Null;
  }
  int compare(const Value &other) const override {
    if (other.type() == ValueType::Null)
      return 0;
    // Define Null < any non-null
    return -1;
  }
};

class IntegerValue final : public Value {
public:
  explicit IntegerValue(int64_t v) : value_(v) {}

  ValueType type() const override { return ValueType::Integer; }
  std::string toString() const override { return std::to_string(value_); }
  std::unique_ptr<Value> clone() const override {
    return std::make_unique<IntegerValue>(value_);
  }

  int64_t asInt() const override { return value_; }
  double asFloat() const override { return static_cast<double>(value_); }
  bool asBool() const override { return value_ != 0; }

  bool equals(const Value &other) const override;
  int compare(const Value &other) const override;

  int64_t value() const { return value_; }

private:
  int64_t value_ = 0;

#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
public:
  static void *operator new(std::size_t sz);
  static void operator delete(void *p) noexcept;
#endif
};

class FloatValue final : public Value {
public:
  explicit FloatValue(double v) : value_(v) {}

  ValueType type() const override { return ValueType::Float; }
  std::string toString() const override;
  std::unique_ptr<Value> clone() const override {
    return std::make_unique<FloatValue>(value_);
  }

  double asFloat() const override { return value_; }
  bool asBool() const override { return value_ != 0.0; }

  bool equals(const Value &other) const override;
  int compare(const Value &other) const override;

  double value() const { return value_; }

private:
  double value_ = 0.0;
};

// StringValue storage model
//
// If KADEDB_RC_STRINGS is defined at build time, StringValue stores its payload
// in a std::shared_ptr<std::string>. This enables reference-counted sharing of
// large string buffers across copies to reduce memory and copy overhead.
//
// Important semantics:
// - clone() remains a DEEP copy (copies the underlying string contents) to
//   preserve deep-copy behavior in APIs that rely on Value::clone().
// - value()/asString() return a const std::string& regardless of storage mode.
// - equals()/compare() and toString() are content-based and unaffected by the
//   storage choice.
//
// Build-time control: -DKADEDB_RC_STRINGS=ON/OFF (default OFF)
class StringValue final : public Value {
public:
  explicit StringValue(std::string v)
#ifdef KADEDB_RC_STRINGS
      : value_(std::make_shared<std::string>(std::move(v))){}
#else
      : value_(std::move(v)) {
  }
#endif

        ValueType type() const override {
    return ValueType::String;
  }
  std::string toString() const override { return '"' + asString() + '"'; }
  // clone() remains deep to preserve deep-copy semantics
  std::unique_ptr<Value> clone() const override {
    return std::make_unique<StringValue>(asString());
  }

#ifdef KADEDB_RC_STRINGS
  const std::string &asString() const override { return *value_; }
  bool asBool() const override { return !value_->empty(); }
#else
  const std::string &asString() const override { return value_; }
  bool asBool() const override { return !value_.empty(); }
#endif

  bool equals(const Value &other) const override;
  int compare(const Value &other) const override;

  const std::string &value() const {
#ifdef KADEDB_RC_STRINGS
    return *value_;
#else
    return value_;
#endif
  }

private:
  // Optionally reference-counted string buffer
#ifdef KADEDB_RC_STRINGS
  std::shared_ptr<std::string> value_;
#else
  std::string value_;
#endif
};

class BooleanValue final : public Value {
public:
  explicit BooleanValue(bool v) : value_(v) {}

  ValueType type() const override { return ValueType::Boolean; }
  std::string toString() const override { return value_ ? "true" : "false"; }
  std::unique_ptr<Value> clone() const override {
    return std::make_unique<BooleanValue>(value_);
  }

  bool asBool() const override { return value_; }
  int64_t asInt() const override { return value_ ? 1 : 0; }
  double asFloat() const override { return value_ ? 1.0 : 0.0; }

  bool equals(const Value &other) const override;
  int compare(const Value &other) const override;

  bool value() const { return value_; }

private:
  bool value_ = false;

#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
public:
  static void *operator new(std::size_t sz);
  static void operator delete(void *p) noexcept;
#endif
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
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

} // namespace kadedb
