#include "kadedb/value.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace kadedb {

// ----- IntegerValue -----
bool IntegerValue::equals(const Value& other) const {
  if (other.type() == ValueType::Integer) {
    return value_ == static_cast<const IntegerValue&>(other).value_;
  }
  if (other.type() == ValueType::Float) {
    return static_cast<double>(value_) == static_cast<const FloatValue&>(other).value();
  }
  return false;
}

int IntegerValue::compare(const Value& other) const {
  if (other.type() == ValueType::Integer) {
    auto ov = static_cast<const IntegerValue&>(other).value_;
    if (value_ < ov) return -1;
    if (value_ > ov) return 1;
    return 0;
  }
  if (other.type() == ValueType::Float) {
    return compareNumeric(static_cast<double>(value_), static_cast<const FloatValue&>(other).value());
  }
  // Define ordering by ValueType for non-numeric comparisons
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// ----- FloatValue -----
bool FloatValue::equals(const Value& other) const {
  if (other.type() == ValueType::Float) {
    return value_ == static_cast<const FloatValue&>(other).value_;
  }
  if (other.type() == ValueType::Integer) {
    return value_ == static_cast<double>(static_cast<const IntegerValue&>(other).value());
  }
  return false;
}

int FloatValue::compare(const Value& other) const {
  if (other.type() == ValueType::Float) {
    return compareNumeric(value_, static_cast<const FloatValue&>(other).value_);
  }
  if (other.type() == ValueType::Integer) {
    return compareNumeric(value_, static_cast<double>(static_cast<const IntegerValue&>(other).value()));
  }
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

std::string FloatValue::toString() const {
  // Simple formatting; keep it minimal and stable
  std::ostringstream oss;
  oss << std::setprecision(15) << value_;
  return oss.str();
}

// ----- StringValue -----
bool StringValue::equals(const Value& other) const {
  if (other.type() != ValueType::String) return false;
  return value_ == static_cast<const StringValue&>(other).value_;
}

int StringValue::compare(const Value& other) const {
  if (other.type() == ValueType::String) {
    const auto& ov = static_cast<const StringValue&>(other).value_;
    if (value_ < ov) return -1;
    if (value_ > ov) return 1;
    return 0;
  }
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// ----- BooleanValue -----
bool BooleanValue::equals(const Value& other) const {
  if (other.type() == ValueType::Boolean) {
    return value_ == static_cast<const BooleanValue&>(other).value_;
  }
  return false;
}

int BooleanValue::compare(const Value& other) const {
  if (other.type() == ValueType::Boolean) {
    bool ov = static_cast<const BooleanValue&>(other).value_;
    if (value_ == ov) return 0;
    return value_ ? 1 : -1;
  }
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// ----- ValueFactory -----
std::unique_ptr<Value> ValueFactory::createNull() { return std::make_unique<NullValue>(); }
std::unique_ptr<Value> ValueFactory::createInteger(int64_t v) { return std::make_unique<IntegerValue>(v); }
std::unique_ptr<Value> ValueFactory::createFloat(double v) { return std::make_unique<FloatValue>(v); }
std::unique_ptr<Value> ValueFactory::createString(std::string v) { return std::make_unique<StringValue>(std::move(v)); }
std::unique_ptr<Value> ValueFactory::createBoolean(bool v) { return std::make_unique<BooleanValue>(v); }

} // namespace kadedb
