#include "kadedb/kadedb_ffi_helpers.h"
#include "kadedb/kadedb.h"
#include "kadedb/schema.h"
#include "kadedb/value.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace kadedb;

// ============================================================================
// OPAQUE HANDLE IMPLEMENTATIONS
// ============================================================================

struct KDB_ValueHandle {
  std::unique_ptr<Value> impl;

  explicit KDB_ValueHandle(std::unique_ptr<Value> v) : impl(std::move(v)) {}
};

struct KDB_Row {
  Row impl;

  explicit KDB_Row(size_t col_count) : impl(col_count) {}
  explicit KDB_Row(Row r) : impl(std::move(r)) {}
};

struct KDB_RowShallow {
  RowShallow impl;

  explicit KDB_RowShallow(size_t col_count) : impl(col_count) {}
  explicit KDB_RowShallow(RowShallow r) : impl(std::move(r)) {}
};

// ----------------------------------------------------------------------------
// Local C++ helper used by various C APIs
// ----------------------------------------------------------------------------
static std::unique_ptr<Value> from_c_value(const KDB_Value &v) {
  switch (v.type) {
  case KDB_VAL_NULL:
    return ValueFactory::createNull();
  case KDB_VAL_INTEGER:
    return ValueFactory::createInteger(static_cast<int64_t>(v.as.i64));
  case KDB_VAL_FLOAT:
    return ValueFactory::createFloat(v.as.f64);
  case KDB_VAL_STRING:
    return ValueFactory::createString(v.as.str ? std::string(v.as.str)
                                               : std::string());
  case KDB_VAL_BOOLEAN:
    return ValueFactory::createBoolean(v.as.boolean != 0);
  default:
    return ValueFactory::createNull();
  }
}

// ============================================================================
// ERROR HANDLING IMPLEMENTATION
// ============================================================================

extern "C" {

const char *kadedb_error_code_string(KDB_ErrorCode code) {
  switch (code) {
  case KDB_SUCCESS:
    return "Success";
  case KDB_ERROR_INVALID_ARGUMENT:
    return "Invalid argument";
  case KDB_ERROR_OUT_OF_RANGE:
    return "Out of range";
  case KDB_ERROR_DUPLICATE_NAME:
    return "Duplicate name";
  case KDB_ERROR_NOT_FOUND:
    return "Not found";
  case KDB_ERROR_VALIDATION_FAILED:
    return "Validation failed";
  case KDB_ERROR_MEMORY_ALLOCATION:
    return "Memory allocation failed";
  case KDB_ERROR_TYPE_MISMATCH:
    return "Type mismatch";
  case KDB_ERROR_CONSTRAINT_VIOLATION:
    return "Constraint violation";
  case KDB_ERROR_SCHEMA_CONFLICT:
    return "Schema conflict";
  case KDB_ERROR_SERIALIZATION:
    return "Serialization error";
  case KDB_ERROR_IO:
    return "I/O error";
  case KDB_ERROR_UNKNOWN:
    return "Unknown error";
  default:
    return "Unrecognized error code";
  }
}

// keep extern "C" open for other functions below

void kadedb_set_error(KDB_ErrorInfo *error, KDB_ErrorCode code,
                      const char *message, const char *context, int line) {
  if (!error)
    return;

  error->code = code;
  error->line = line;

  if (message) {
    std::strncpy(error->message, message, sizeof(error->message) - 1);
    error->message[sizeof(error->message) - 1] = '\0';
  } else {
    error->message[0] = '\0';
  }

  if (context) {
    std::strncpy(error->context, context, sizeof(error->context) - 1);
    error->context[sizeof(error->context) - 1] = '\0';
  } else {
    error->context[0] = '\0';
  }
}

// ============================================================================
// RESOURCE MANAGEMENT IMPLEMENTATION
// ============================================================================

int kadedb_resource_manager_init(KDB_ResourceManager *manager,
                                 size_t initial_capacity) {
  if (!manager)
    return 0;

  manager->resources =
      static_cast<void **>(std::malloc(initial_capacity * sizeof(void *)));
  manager->destructors = static_cast<void (**)(void *)>(
      std::malloc(initial_capacity * sizeof(void (*)(void *))));

  if (!manager->resources || !manager->destructors) {
    std::free(manager->resources);
    std::free(manager->destructors);
    return 0;
  }

  manager->count = 0;
  manager->capacity = initial_capacity;
  return 1;
}

int kadedb_resource_manager_add(KDB_ResourceManager *manager, void *resource,
                                void (*destructor)(void *)) {
  if (!manager || !resource || !destructor)
    return 0;

  if (manager->count >= manager->capacity) {
    size_t new_capacity = manager->capacity * 2;
    void **new_resources = static_cast<void **>(
        std::realloc(manager->resources, new_capacity * sizeof(void *)));
    void (**new_destructors)(void *) =
        static_cast<void (**)(void *)>(std::realloc(
            manager->destructors, new_capacity * sizeof(void (*)(void *))));

    if (!new_resources || !new_destructors) {
      std::free(new_resources);
      std::free(new_destructors);
      return 0;
    }

    manager->resources = new_resources;
    manager->destructors = new_destructors;
    manager->capacity = new_capacity;
  }

  manager->resources[manager->count] = resource;
  manager->destructors[manager->count] = destructor;
  manager->count++;
  return 1;
}

void kadedb_resource_manager_cleanup(KDB_ResourceManager *manager) {
  if (!manager)
    return;

  for (size_t i = 0; i < manager->count; ++i) {
    if (manager->resources[i] && manager->destructors[i]) {
      manager->destructors[i](manager->resources[i]);
    }
  }

  std::free(manager->resources);
  std::free(manager->destructors);

  manager->resources = nullptr;
  manager->destructors = nullptr;
  manager->count = 0;
  manager->capacity = 0;
}

// ============================================================================
// VALUE HANDLE IMPLEMENTATION
// ============================================================================

KDB_ValueHandle *KadeDB_Value_CreateNull() {
  try {
    return new KDB_ValueHandle(ValueFactory::createNull());
  } catch (...) {
    return nullptr;
  }
}

KDB_ValueHandle *kadedb_value_to_handle(const KDB_Value *c_value,
                                        KDB_ErrorInfo *error) {
  kadedb_clear_error(error);
  if (!c_value) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "c_value is null");
    return nullptr;
  }
  try {
    return new KDB_ValueHandle(from_c_value(*c_value));
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return nullptr;
  }
}

int kadedb_handle_to_value(const KDB_ValueHandle *handle, KDB_Value *out_value,
                           KDB_ErrorInfo *error) {
  kadedb_clear_error(error);
  if (!handle || !handle->impl || !out_value) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "handle or out_value is null");
    return 0;
  }
  try {
    // Use a thread-local cache for string data to keep the c_str() valid
    // after this function returns (until the next call on this thread).
    // This avoids requiring the caller to free memory and prevents dangling
    // pointers in optimized builds.
    thread_local std::string __kadedb_string_tls_cache;
    switch (handle->impl->type()) {
    case ValueType::Null:
      out_value->type = KDB_VAL_NULL;
      break;
    case ValueType::Integer:
      out_value->type = KDB_VAL_INTEGER;
      out_value->as.i64 = static_cast<long long>(handle->impl->asInt());
      break;
    case ValueType::Float:
      out_value->type = KDB_VAL_FLOAT;
      out_value->as.f64 = handle->impl->asFloat();
      break;
    case ValueType::String:
      out_value->type = KDB_VAL_STRING;
      __kadedb_string_tls_cache = handle->impl->asString();
      out_value->as.str = __kadedb_string_tls_cache.c_str();
      break;
    case ValueType::Boolean:
      out_value->type = KDB_VAL_BOOLEAN;
      out_value->as.boolean = handle->impl->asBool() ? 1 : 0;
      break;
    default:
      out_value->type = KDB_VAL_NULL;
      break;
    }
    return 1;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return 0;
  }
}

int kadedb_create_document(const char **keys, const KDB_Value *values,
                           unsigned long long count, KDB_KeyValue **out_doc,
                           KDB_ErrorInfo *error) {
  kadedb_clear_error(error);
  if (!out_doc) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "out_doc is null");
    return 0;
  }
  if ((count > 0) && (!keys || !values)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "keys or values array is null");
    return 0;
  }
  KDB_KeyValue *doc =
      static_cast<KDB_KeyValue *>(std::malloc(count * sizeof(KDB_KeyValue)));
  if (!doc) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to allocate document array");
    return 0;
  }
  // Deep copy keys and string values
  for (unsigned long long i = 0; i < count; ++i) {
    const char *k = keys ? keys[i] : nullptr;
    if (k) {
      size_t len = std::strlen(k);
      char *kdup = static_cast<char *>(std::malloc(len + 1));
      if (!kdup) {
        // rollback
        for (unsigned long long j = 0; j < i; ++j) {
          std::free((void *)doc[j].key);
          if (doc[j].value.type == KDB_VAL_STRING) {
            std::free((void *)doc[j].value.as.str);
          }
        }
        std::free(doc);
        KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate key string");
        return 0;
      }
      std::strcpy(kdup, k);
      doc[i].key = kdup;
    } else {
      doc[i].key = nullptr;
    }

    // Copy value; deep copy string content
    doc[i].value = values ? values[i] : KDB_Value{KDB_VAL_NULL, {0}};
    if (doc[i].value.type == KDB_VAL_STRING && doc[i].value.as.str) {
      const char *sv = doc[i].value.as.str;
      size_t slen = std::strlen(sv);
      char *sdup = static_cast<char *>(std::malloc(slen + 1));
      if (!sdup) {
        for (unsigned long long j = 0; j <= i; ++j) {
          std::free((void *)doc[j].key);
          if (doc[j].value.type == KDB_VAL_STRING) {
            std::free((void *)doc[j].value.as.str);
          }
        }
        std::free(doc);
        KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate value string");
        return 0;
      }
      std::strcpy(sdup, sv);
      doc[i].value.as.str = sdup;
    }
  }
  *out_doc = doc;
  return 1;
}

void kadedb_free_document(KDB_KeyValue *doc, unsigned long long count) {
  if (!doc)
    return;
  for (unsigned long long i = 0; i < count; ++i) {
    std::free((void *)doc[i].key);
    if (doc[i].value.type == KDB_VAL_STRING) {
      std::free((void *)doc[i].value.as.str);
    }
  }
  std::free(doc);
}

KDB_ValueHandle *KadeDB_Value_CreateInteger(long long value) {
  try {
    return new KDB_ValueHandle(
        ValueFactory::createInteger(static_cast<int64_t>(value)));
  } catch (...) {
    return nullptr;
  }
}

KDB_ValueHandle *KadeDB_Value_CreateFloat(double value) {
  try {
    return new KDB_ValueHandle(ValueFactory::createFloat(value));
  } catch (...) {
    return nullptr;
  }
}

KDB_ValueHandle *KadeDB_Value_CreateString(const char *value) {
  try {
    std::string str = value ? std::string(value) : std::string();
    return new KDB_ValueHandle(ValueFactory::createString(std::move(str)));
  } catch (...) {
    return nullptr;
  }
}

KDB_ValueHandle *KadeDB_Value_CreateBoolean(int value) {
  try {
    return new KDB_ValueHandle(ValueFactory::createBoolean(value != 0));
  } catch (...) {
    return nullptr;
  }
}

void KadeDB_Value_Destroy(KDB_ValueHandle *value) { delete value; }

KDB_ValueType KadeDB_Value_GetType(const KDB_ValueHandle *value) {
  if (!value || !value->impl)
    return KDB_VAL_NULL;

  switch (value->impl->type()) {
  case ValueType::Null:
    return KDB_VAL_NULL;
  case ValueType::Integer:
    return KDB_VAL_INTEGER;
  case ValueType::Float:
    return KDB_VAL_FLOAT;
  case ValueType::String:
    return KDB_VAL_STRING;
  case ValueType::Boolean:
    return KDB_VAL_BOOLEAN;
  default:
    return KDB_VAL_NULL;
  }
}

int KadeDB_Value_IsNull(const KDB_ValueHandle *value) {
  if (!value || !value->impl)
    return 1;
  return value->impl->type() == ValueType::Null ? 1 : 0;
}

long long KadeDB_Value_AsInteger(const KDB_ValueHandle *value,
                                 KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return 0;
  }

  try {
    return static_cast<long long>(value->impl->asInt());
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, e.what());
    return 0;
  }
}

double KadeDB_Value_AsFloat(const KDB_ValueHandle *value,
                            KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return 0.0;
  }

  try {
    return value->impl->asFloat();
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, e.what());
    return 0.0;
  }
}

const char *KadeDB_Value_AsString(const KDB_ValueHandle *value,
                                  KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return nullptr;
  }

  try {
    return value->impl->asString().c_str();
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, e.what());
    return nullptr;
  }
}

int KadeDB_Value_AsBoolean(const KDB_ValueHandle *value, KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return 0;
  }

  try {
    return value->impl->asBool() ? 1 : 0;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, e.what());
    return 0;
  }
}

KDB_ValueHandle *KadeDB_Value_Clone(const KDB_ValueHandle *value) {
  if (!value || !value->impl)
    return nullptr;

  try {
    return new KDB_ValueHandle(value->impl->clone());
  } catch (...) {
    return nullptr;
  }
}

int KadeDB_Value_Equals(const KDB_ValueHandle *a, const KDB_ValueHandle *b) {
  if (!a || !b || !a->impl || !b->impl)
    return 0;
  return a->impl->equals(*b->impl) ? 1 : 0;
}

int KadeDB_Value_Compare(const KDB_ValueHandle *a, const KDB_ValueHandle *b) {
  if (!a || !b || !a->impl || !b->impl)
    return 0;
  return a->impl->compare(*b->impl);
}

char *KadeDB_Value_ToString(const KDB_ValueHandle *value) {
  if (!value || !value->impl)
    return nullptr;

  try {
    std::string str = value->impl->toString();
    char *result = static_cast<char *>(std::malloc(str.length() + 1));
    if (result) {
      std::strcpy(result, str.c_str());
    }
    return result;
  } catch (...) {
    return nullptr;
  }
}

// ============================================================================
// ROW HANDLE IMPLEMENTATION
// ============================================================================

KDB_Row *KadeDB_Row_Create(unsigned long long column_count) {
  try {
    return new KDB_Row(static_cast<size_t>(column_count));
  } catch (...) {
    return nullptr;
  }
}

void KadeDB_Row_Destroy(KDB_Row *row) { delete row; }

KDB_Row *KadeDB_Row_Clone(const KDB_Row *row) {
  if (!row)
    return nullptr;

  try {
    return new KDB_Row(row->impl.clone());
  } catch (...) {
    return nullptr;
  }
}

unsigned long long KadeDB_Row_Size(const KDB_Row *row) {
  if (!row)
    return 0;
  return static_cast<unsigned long long>(row->impl.size());
}

int KadeDB_Row_Set(KDB_Row *row, unsigned long long index,
                   const KDB_ValueHandle *value, KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!row) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Row is null");
    return 0;
  }

  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return 0;
  }

  try {
    row->impl.set(static_cast<size_t>(index), value->impl->clone());
    return 1;
  } catch (const std::out_of_range &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_OUT_OF_RANGE, e.what());
    return 0;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return 0;
  }
}

KDB_ValueHandle *KadeDB_Row_Get(const KDB_Row *row, unsigned long long index,
                                KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!row) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Row is null");
    return nullptr;
  }

  try {
    const Value &val = row->impl.at(static_cast<size_t>(index));
    return new KDB_ValueHandle(val.clone());
  } catch (const std::out_of_range &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_OUT_OF_RANGE, e.what());
    return nullptr;
  } catch (...) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN,
                     "Unknown error getting row value");
    return nullptr;
  }
}

// ============================================================================
// ROW SHALLOW HANDLE IMPLEMENTATION
// ============================================================================

KDB_RowShallow *KadeDB_RowShallow_Create(unsigned long long column_count) {
  try {
    return new KDB_RowShallow(static_cast<size_t>(column_count));
  } catch (...) {
    return nullptr;
  }
}

void KadeDB_RowShallow_Destroy(KDB_RowShallow *row) { delete row; }

KDB_RowShallow *KadeDB_RowShallow_FromRow(const KDB_Row *row) {
  if (!row)
    return nullptr;
  try {
    return new KDB_RowShallow(RowShallow::fromClones(row->impl));
  } catch (...) {
    return nullptr;
  }
}

KDB_Row *KadeDB_RowShallow_ToRow(const KDB_RowShallow *row) {
  if (!row)
    return nullptr;
  try {
    return new KDB_Row(row->impl.toRowDeep());
  } catch (...) {
    return nullptr;
  }
}

unsigned long long KadeDB_RowShallow_Size(const KDB_RowShallow *row) {
  if (!row)
    return 0ULL;
  return static_cast<unsigned long long>(row->impl.size());
}

int KadeDB_RowShallow_Set(KDB_RowShallow *row, unsigned long long index,
                          const KDB_ValueHandle *value, KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!row) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "RowShallow is null");
    return 0;
  }
  if (!value || !value->impl) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
    return 0;
  }
  try {
    row->impl.set(static_cast<size_t>(index),
                  std::shared_ptr<Value>(value->impl->clone().release()));
    return 1;
  } catch (const std::out_of_range &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_OUT_OF_RANGE, e.what());
    return 0;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return 0;
  }
}

KDB_ValueHandle *KadeDB_RowShallow_Get(const KDB_RowShallow *row,
                                       unsigned long long index,
                                       KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!row) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "RowShallow is null");
    return nullptr;
  }
  try {
    size_t idx = static_cast<size_t>(index);
    // Bounds check via size()
    if (idx >= row->impl.size()) {
      throw std::out_of_range("RowShallow index out of range");
    }
    // Access underlying shared_ptr without dereferencing through at()
    const auto &cellPtr = row->impl.values().at(idx);
    if (!cellPtr) {
      KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                       "RowShallow cell is null");
      return nullptr;
    }
    return new KDB_ValueHandle(cellPtr->clone());
  } catch (const std::out_of_range &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_OUT_OF_RANGE, e.what());
    return nullptr;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return nullptr;
  }
}

// ============================================================================
// STRING MEMORY MANAGEMENT
// ============================================================================

void KadeDB_String_Free(char *str) { std::free(str); }

char *KadeDB_String_Duplicate(const char *str) {
  if (!str)
    return nullptr;

  size_t len = std::strlen(str);
  char *result = static_cast<char *>(std::malloc(len + 1));
  if (result) {
    std::strcpy(result, str);
  }
  return result;
}

// ============================================================================
// CONVENIENCE HELPERS
// =========================================================================

KDB_Row *kadedb_create_row_with_values(const KDB_Value *values,
                                       unsigned long long count,
                                       KDB_ErrorInfo *error) {
  if (!values && count > 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Values array is null");
    return nullptr;
  }

  try {
    KDB_Row *row = new KDB_Row(static_cast<size_t>(count));

    for (unsigned long long i = 0; i < count; ++i) {
      row->impl.set(static_cast<size_t>(i), from_c_value(values[i]));
    }

    return row;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return nullptr;
  }
}

static void to_c_value(const Value &cpp_val, KDB_Value &c_val) {
  switch (cpp_val.type()) {
  case ValueType::Null:
    c_val.type = KDB_VAL_NULL;
    break;
  case ValueType::Integer:
    c_val.type = KDB_VAL_INTEGER;
    c_val.as.i64 = static_cast<long long>(cpp_val.asInt());
    break;
  case ValueType::Float:
    c_val.type = KDB_VAL_FLOAT;
    c_val.as.f64 = cpp_val.asFloat();
    break;
  case ValueType::String:
    c_val.type = KDB_VAL_STRING;
    // Note: This creates a dangling pointer - caller must manage string
    // lifetime
    c_val.as.str = cpp_val.asString().c_str();
    break;
  case ValueType::Boolean:
    c_val.type = KDB_VAL_BOOLEAN;
    c_val.as.boolean = cpp_val.asBool() ? 1 : 0;
    break;
  default:
    c_val.type = KDB_VAL_NULL;
    break;
  }
}

KDB_Value *kadedb_row_to_value_array(const KDB_Row *row,
                                     unsigned long long *out_count,
                                     KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!row || !out_count) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "Row or count pointer is null");
    return nullptr;
  }

  try {
    size_t count = row->impl.size();
    *out_count = static_cast<unsigned long long>(count);

    KDB_Value *values =
        static_cast<KDB_Value *>(std::malloc(count * sizeof(KDB_Value)));
    if (!values) {
      KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                       "Failed to allocate value array");
      return nullptr;
    }

    for (size_t i = 0; i < count; ++i) {
      to_c_value(row->impl.at(i), values[i]);
    }

    return values;
  } catch (const std::exception &e) {
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, e.what());
    return nullptr;
  }
}

void kadedb_free_value_array(KDB_Value *values, unsigned long long count) {
  (void)count; // Suppress unused parameter warning
  std::free(values);
}

// ============================================================================
// DEBUGGING AND DIAGNOSTICS
// ============================================================================

#ifdef KADEDB_MEM_DEBUG
int kadedb_get_memory_info(KDB_MemoryInfo *info) {
  if (!info)
    return 0;

  // Use the memory debug counters from the Value system
  info->total_allocated = memdebug::allocCountInteger() +
                          memdebug::allocCountBoolean() +
                          memdebug::allocCountNull();
  info->total_freed = memdebug::freeCountInteger() +
                      memdebug::freeCountBoolean() + memdebug::freeCountNull();
  info->current_usage = info->total_allocated - info->total_freed;
  info->peak_usage = info->current_usage; // Simplified - actual peak tracking
                                          // would need more work

  return 1;
}

void kadedb_print_memory_stats() {
  printf("=== KadeDB Memory Statistics ===\n");
  printf("Integer allocations: %zu, frees: %zu\n",
         memdebug::allocCountInteger(), memdebug::freeCountInteger());
  printf("Boolean allocations: %zu, frees: %zu\n",
         memdebug::allocCountBoolean(), memdebug::freeCountBoolean());
  printf("Null allocations: %zu, frees: %zu\n", memdebug::allocCountNull(),
         memdebug::freeCountNull());
}

int kadedb_check_resource_leaks() {
  size_t total_alloc = memdebug::allocCountInteger() +
                       memdebug::allocCountBoolean() +
                       memdebug::allocCountNull();
  size_t total_free = memdebug::freeCountInteger() +
                      memdebug::freeCountBoolean() + memdebug::freeCountNull();
  return (total_alloc != total_free) ? 1 : 0;
}
#else
int kadedb_get_memory_info(KDB_MemoryInfo *info) {
  if (!info)
    return 0;
  info->total_allocated = 0;
  info->total_freed = 0;
  info->current_usage = 0;
  info->peak_usage = 0;
  return 0; // Not available without KADEDB_MEM_DEBUG
}

void kadedb_print_memory_stats() {
  printf(
      "Memory debugging not enabled. Build with KADEDB_MEM_DEBUG to enable.\n");
}

int kadedb_check_resource_leaks() {
  return 0; // Cannot detect leaks without debug mode
}
#endif

} // extern "C"
