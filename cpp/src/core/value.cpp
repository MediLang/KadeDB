#include "kadedb/value.h"

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <new>
#include <sstream>

namespace kadedb {

#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
namespace {
struct FreeNode {
  FreeNode *next;
};

// Tiny freelists per type (not thread-safe; intended for debug/micro perf
// experiments)
static FreeNode *g_pool_int = nullptr;
static FreeNode *g_pool_bool = nullptr;
static FreeNode *g_pool_null = nullptr;

// Counters
static size_t g_alloc_int = 0, g_free_int = 0;
static size_t g_alloc_bool = 0, g_free_bool = 0;
static size_t g_alloc_null = 0, g_free_null = 0;

inline void *pool_alloc(FreeNode *&head, std::size_t sz, size_t &allocCounter) {
  ++allocCounter;
#ifdef KADEDB_ENABLE_SMALL_OBJECT_POOL
  if (head) {
    void *p = head;
    head = head->next;
    return p;
  }
#endif
  return ::operator new(sz);
}
inline void pool_free(FreeNode *&head, void *p, size_t &freeCounter) noexcept {
  ++freeCounter;
#ifdef KADEDB_ENABLE_SMALL_OBJECT_POOL
  auto *node = static_cast<FreeNode *>(p);
  node->next = head;
  head = node;
  return;
#endif
  ::operator delete(p);
}
} // namespace

namespace memdebug {
size_t allocCountInteger() { return g_alloc_int; }
size_t freeCountInteger() { return g_free_int; }
size_t allocCountBoolean() { return g_alloc_bool; }
size_t freeCountBoolean() { return g_free_bool; }
size_t allocCountNull() { return g_alloc_null; }
size_t freeCountNull() { return g_free_null; }
} // namespace memdebug
#endif

// ----- IntegerValue -----
bool IntegerValue::equals(const Value &other) const {
  if (other.type() == ValueType::Integer) {
    return value_ == static_cast<const IntegerValue &>(other).value_;
  }
  if (other.type() == ValueType::Float) {
    return static_cast<double>(value_) ==
           static_cast<const FloatValue &>(other).value();
  }
  return false;
}

int IntegerValue::compare(const Value &other) const {
  if (other.type() == ValueType::Integer) {
    auto ov = static_cast<const IntegerValue &>(other).value_;
    if (value_ < ov)
      return -1;
    if (value_ > ov)
      return 1;
    return 0;
  }
  if (other.type() == ValueType::Float) {
    return compareNumeric(static_cast<double>(value_),
                          static_cast<const FloatValue &>(other).value());
  }
  // Define ordering by ValueType for non-numeric comparisons
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// Custom allocators (optional)
#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
void *IntegerValue::operator new(std::size_t sz) {
  return pool_alloc(g_pool_int, sz, g_alloc_int);
}
void IntegerValue::operator delete(void *p) noexcept {
  pool_free(g_pool_int, p, g_free_int);
}
#endif

// ----- FloatValue -----
bool FloatValue::equals(const Value &other) const {
  if (other.type() == ValueType::Float) {
    return value_ == static_cast<const FloatValue &>(other).value_;
  }
  if (other.type() == ValueType::Integer) {
    return value_ == static_cast<double>(
                         static_cast<const IntegerValue &>(other).value());
  }
  return false;
}

int FloatValue::compare(const Value &other) const {
  if (other.type() == ValueType::Float) {
    return compareNumeric(value_,
                          static_cast<const FloatValue &>(other).value_);
  }
  if (other.type() == ValueType::Integer) {
    return compareNumeric(
        value_,
        static_cast<double>(static_cast<const IntegerValue &>(other).value()));
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
bool StringValue::equals(const Value &other) const {
  if (other.type() != ValueType::String)
    return false;
  const auto &ov = static_cast<const StringValue &>(other).asString();
  return asString() == ov;
}

int StringValue::compare(const Value &other) const {
  if (other.type() == ValueType::String) {
    const auto &lv = asString();
    const auto &ov = static_cast<const StringValue &>(other).asString();
    if (lv < ov)
      return -1;
    if (lv > ov)
      return 1;
    return 0;
  }
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// ----- BooleanValue -----
bool BooleanValue::equals(const Value &other) const {
  if (other.type() == ValueType::Boolean) {
    return value_ == static_cast<const BooleanValue &>(other).value_;
  }
  return false;
}

int BooleanValue::compare(const Value &other) const {
  if (other.type() == ValueType::Boolean) {
    bool ov = static_cast<const BooleanValue &>(other).value_;
    if (value_ == ov)
      return 0;
    return value_ ? 1 : -1;
  }
  return static_cast<int>(type()) - static_cast<int>(other.type());
}

// Custom allocators (optional)
#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
void *BooleanValue::operator new(std::size_t sz) {
  return pool_alloc(g_pool_bool, sz, g_alloc_bool);
}
void BooleanValue::operator delete(void *p) noexcept {
  pool_free(g_pool_bool, p, g_free_bool);
}
#endif

// ----- ValueFactory -----
std::unique_ptr<Value> ValueFactory::createNull() {
  return std::make_unique<NullValue>();
}
std::unique_ptr<Value> ValueFactory::createInteger(int64_t v) {
  return std::make_unique<IntegerValue>(v);
}
std::unique_ptr<Value> ValueFactory::createFloat(double v) {
  return std::make_unique<FloatValue>(v);
}
std::unique_ptr<Value> ValueFactory::createString(std::string v) {
  return std::make_unique<StringValue>(std::move(v));
}
std::unique_ptr<Value> ValueFactory::createBoolean(bool v) {
  return std::make_unique<BooleanValue>(v);
}

// ----- NullValue custom allocator -----
#if defined(KADEDB_MEM_DEBUG) || defined(KADEDB_ENABLE_SMALL_OBJECT_POOL)
void *NullValue::operator new(std::size_t sz) {
  return pool_alloc(g_pool_null, sz, g_alloc_null);
}
void NullValue::operator delete(void *p) noexcept {
  pool_free(g_pool_null, p, g_free_null);
}
#endif

} // namespace kadedb
