#include "kadedb/storage.h"

#include <unordered_set>

namespace kadedb {

// Utility: evaluate a comparison predicate on a row
static bool evalPredicateComparison(const TableSchema &schema, const Row &row,
                                    const Predicate &pred) {
  size_t idx = schema.findColumn(pred.column);
  if (idx == TableSchema::npos)
    return false; // unknown column -> not matched
  const Value *lhs = row.values()[idx].get();
  const Value *rhs = pred.rhs.get();
  if (!lhs || !rhs)
    return false; // null comparisons -> no match (semantics retained)
  int cmp = lhs->compare(*rhs);
  switch (pred.op) {
  case Predicate::Op::Eq:
    return cmp == 0;
  case Predicate::Op::Ne:
    return cmp != 0;
  case Predicate::Op::Lt:
    return cmp < 0;
  case Predicate::Op::Le:
    return cmp <= 0;
  case Predicate::Op::Gt:
    return cmp > 0;
  case Predicate::Op::Ge:
    return cmp >= 0;
  }
  return false;
}

// Utility: evaluate predicate tree (supports And/Or/Not and Comparison)
static bool evalPredicate(const TableSchema &schema, const Row &row,
                          const Predicate &pred) {
  using K = Predicate::Kind;
  switch (pred.kind) {
  case K::Comparison:
    return evalPredicateComparison(schema, row, pred);
  case K::And: {
    // AND with zero children -> true (neutral element)
    for (const auto &ch : pred.children) {
      if (!evalPredicate(schema, row, ch))
        return false;
    }
    return true;
  }
  case K::Or: {
    // OR with zero children -> false (neutral element)
    for (const auto &ch : pred.children) {
      if (evalPredicate(schema, row, ch))
        return true;
    }
    return false;
  }
  case K::Not: {
    // NOT expects exactly one child; if none, treat as true negated -> false
    if (pred.children.empty())
      return false;
    return !evalPredicate(schema, row, pred.children.front());
  }
  }
  return false;
}

// Utility: evaluate document predicate comparison
static bool evalDocPredicateComparison(const Document &doc,
                                       const DocPredicate &pred) {
  auto it = doc.find(pred.field);
  if (it == doc.end())
    return false; // unknown field -> not matched
  const Value *lhs = it->second.get();
  const Value *rhs = pred.rhs.get();
  if (!lhs || !rhs)
    return false; // null comparisons -> no match
  int cmp = lhs->compare(*rhs);
  switch (pred.op) {
  case DocPredicate::Op::Eq:
    return cmp == 0;
  case DocPredicate::Op::Ne:
    return cmp != 0;
  case DocPredicate::Op::Lt:
    return cmp < 0;
  case DocPredicate::Op::Le:
    return cmp <= 0;
  case DocPredicate::Op::Gt:
    return cmp > 0;
  case DocPredicate::Op::Ge:
    return cmp >= 0;
  }
  return false;
}

// Utility: evaluate document predicate tree (And/Or/Not/Comparison)
static bool evalDocPredicate(const Document &doc, const DocPredicate &pred) {
  using K = DocPredicate::Kind;
  switch (pred.kind) {
  case K::Comparison:
    return evalDocPredicateComparison(doc, pred);
  case K::And: {
    for (const auto &ch : pred.children) {
      if (!evalDocPredicate(doc, ch))
        return false;
    }
    return true;
  }
  case K::Or: {
    for (const auto &ch : pred.children) {
      if (evalDocPredicate(doc, ch))
        return true;
    }
    return false;
  }
  case K::Not: {
    if (pred.children.empty())
      return false;
    return !evalDocPredicate(doc, pred.children.front());
  }
  }
  return false;
}

Status InMemoryRelationalStorage::createTable(const std::string &table,
                                              const TableSchema &schema) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (tables_.find(table) != tables_.end()) {
    return Status::AlreadyExists("Table already exists: " + table);
  }
  tables_.emplace(table, TableData{schema, {}});
  return Status::OK();
}

Status InMemoryRelationalStorage::insertRow(const std::string &table,
                                            const Row &row) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end()) {
    return Status::NotFound("Unknown table: " + table);
  }
  const auto &schema = it->second.schema;
  // Validate row matches schema
  if (auto err = SchemaValidator::validateRow(schema, row); !err.empty()) {
    return Status::InvalidArgument(err);
  }
  // In-memory append (deep copy to keep isolation)
  it->second.rows.push_back(row.clone());

  // Enforce uniqueness constraints after insertion
  if (auto err = SchemaValidator::validateUnique(schema, it->second.rows);
      !err.empty()) {
    // revert append
    it->second.rows.pop_back();
    return Status::FailedPrecondition(err);
  }

  return Status::OK();
}

Result<ResultSet>
InMemoryRelationalStorage::select(const std::string &table,
                                  const std::vector<std::string> &columns,
                                  const std::optional<Predicate> &where) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end()) {
    return Result<ResultSet>::err(Status::NotFound("Unknown table: " + table));
  }
  const auto &schema = it->second.schema;

  // Determine projection indices and column metadata
  std::vector<size_t> projIdx;
  std::vector<std::string> outNames;
  std::vector<ColumnType> outTypes;

  if (columns.empty()) {
    // select *
    const auto &cols = schema.columns();
    projIdx.resize(cols.size());
    for (size_t i = 0; i < cols.size(); ++i) {
      projIdx[i] = i;
      outNames.push_back(cols[i].name);
      outTypes.push_back(cols[i].type);
    }
  } else {
    for (const auto &name : columns) {
      size_t idx = schema.findColumn(name);
      if (idx == TableSchema::npos) {
        return Result<ResultSet>::err(
            Status::InvalidArgument("Unknown column in projection: " + name));
      }
      projIdx.push_back(idx);
      Column col;
      schema.getColumn(name, col);
      outNames.push_back(col.name);
      outTypes.push_back(col.type);
    }
  }

  ResultSet rs(outNames, outTypes);

  for (const auto &r : it->second.rows) {
    if (where) {
      if (!evalPredicate(schema, r, *where))
        continue;
    }
    std::vector<std::unique_ptr<Value>> cells;
    cells.reserve(projIdx.size());
    for (size_t idx : projIdx) {
      const auto &v = r.values()[idx];
      cells.push_back(v ? v->clone() : nullptr);
    }
    rs.addRow(ResultRow(std::move(cells)));
  }

  return Result<ResultSet>::ok(std::move(rs));
}

Status InMemoryDocumentStorage::createCollection(
    const std::string &collection,
    const std::optional<DocumentSchema> &schema) {
  if (data_.find(collection) != data_.end())
    return Status::AlreadyExists("Collection already exists: " + collection);
  CollectionData cd;
  cd.schema = schema;
  data_.emplace(collection, std::move(cd));
  return Status::OK();
}

Status InMemoryDocumentStorage::dropCollection(const std::string &collection) {
  auto it = data_.find(collection);
  if (it == data_.end())
    return Status::NotFound("Unknown collection: " + collection);
  data_.erase(it);
  return Status::OK();
}

std::vector<std::string> InMemoryDocumentStorage::listCollections() const {
  std::vector<std::string> names;
  names.reserve(data_.size());
  for (const auto &kv : data_)
    names.push_back(kv.first);
  return names;
}

Status InMemoryDocumentStorage::put(const std::string &collection,
                                    const std::string &key,
                                    const Document &doc) {
  // Create collection lazily if missing (MVP behavior)
  auto it = data_.find(collection);
  if (it == data_.end()) {
    CollectionData cd;
    data_.emplace(collection, std::move(cd));
    it = data_.find(collection);
  }

  // Validate against schema if present
  if (it->second.schema) {
    auto err = SchemaValidator::validateDocument(*it->second.schema, doc);
    if (!err.empty())
      return Status::InvalidArgument(err);
  }

  // Enforce uniqueness constraints if schema defines unique fields
  if (it->second.schema) {
    std::vector<Document> docs;
    docs.reserve(it->second.docs.size() + 1);
    for (const auto &kv : it->second.docs) {
      if (kv.first == key)
        continue; // skip existing same key; replaced later
      docs.emplace_back(deepCopyDocument(kv.second));
    }
    docs.emplace_back(deepCopyDocument(doc));
    auto err = SchemaValidator::validateUnique(*it->second.schema, docs);
    if (!err.empty())
      return Status::FailedPrecondition(err);
  }

  it->second.docs[key] = deepCopyDocument(doc);
  return Status::OK();
}

Result<Document> InMemoryDocumentStorage::get(const std::string &collection,
                                              const std::string &key) {
  auto cit = data_.find(collection);
  if (cit == data_.end())
    return Result<Document>::err(Status::NotFound("Unknown collection"));
  auto kit = cit->second.docs.find(key);
  if (kit == cit->second.docs.end())
    return Result<Document>::err(Status::NotFound("Key not found"));
  return Result<Document>::ok(deepCopyDocument(kit->second));
}

Status InMemoryDocumentStorage::erase(const std::string &collection,
                                      const std::string &key) {
  auto cit = data_.find(collection);
  if (cit == data_.end())
    return Status::NotFound("Unknown collection: " + collection);
  auto kit = cit->second.docs.find(key);
  if (kit == cit->second.docs.end())
    return Status::NotFound("Key not found: " + key);
  cit->second.docs.erase(kit);
  return Status::OK();
}

Result<size_t>
InMemoryDocumentStorage::count(const std::string &collection) const {
  auto cit = data_.find(collection);
  if (cit == data_.end())
    return Result<size_t>::err(Status::NotFound("Unknown collection"));
  return Result<size_t>::ok(cit->second.docs.size());
}

Result<std::vector<std::pair<std::string, Document>>>
InMemoryDocumentStorage::query(const std::string &collection,
                               const std::vector<std::string> &fields,
                               const std::optional<DocPredicate> &where) {
  auto cit = data_.find(collection);
  if (cit == data_.end())
    return Result<std::vector<std::pair<std::string, Document>>>::err(
        Status::NotFound("Unknown collection"));

  const auto &schemaOpt = cit->second.schema;

  // Validate projection field names if we have a schema
  if (schemaOpt && !fields.empty()) {
    for (const auto &f : fields) {
      if (!schemaOpt->hasField(f)) {
        return Result<std::vector<std::pair<std::string, Document>>>::err(
            Status::InvalidArgument("Unknown field in projection: " + f));
      }
    }
  }

  // Helper to validate predicate fields exist in schema
  auto validateWhereFields = [](const DocumentSchema &schema,
                                const DocPredicate &pred, auto &self) -> bool {
    using K = DocPredicate::Kind;
    switch (pred.kind) {
    case K::Comparison:
      return schema.hasField(pred.field);
    case K::And:
    case K::Or: {
      for (const auto &ch : pred.children) {
        if (!self(schema, ch, self))
          return false;
      }
      return true;
    }
    case K::Not: {
      if (pred.children.empty())
        return true; // treat empty NOT as valid (evaluates to false later)
      return self(schema, pred.children.front(), self);
    }
    }
    return false;
  };

  // Validate predicate fields against schema, if present
  if (schemaOpt && where) {
    if (!validateWhereFields(*schemaOpt, *where, validateWhereFields)) {
      return Result<std::vector<std::pair<std::string, Document>>>::err(
          Status::InvalidArgument("Unknown field in predicate"));
    }
  }

  std::vector<std::pair<std::string, Document>> out;
  out.reserve(cit->second.docs.size());
  for (const auto &kv : cit->second.docs) {
    const auto &k = kv.first;
    const auto &doc = kv.second;
    if (where) {
      if (!evalDocPredicate(doc, *where))
        continue;
    }
    if (fields.empty()) {
      out.emplace_back(k, deepCopyDocument(doc));
    } else {
      Document proj;
      for (const auto &fname : fields) {
        auto it = doc.find(fname);
        if (it != doc.end()) {
          proj.emplace(fname, it->second ? it->second->clone() : nullptr);
        }
      }
      out.emplace_back(k, std::move(proj));
    }
  }

  return Result<std::vector<std::pair<std::string, Document>>>::ok(
      std::move(out));
}

std::vector<std::string> InMemoryRelationalStorage::listTables() const {
  std::lock_guard<std::mutex> lk(mtx_);
  std::vector<std::string> names;
  names.reserve(tables_.size());
  for (const auto &kv : tables_)
    names.push_back(kv.first);
  return names;
}

Status InMemoryRelationalStorage::dropTable(const std::string &table) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end())
    return Status::NotFound("Unknown table: " + table);
  tables_.erase(it);
  return Status::OK();
}

Result<size_t>
InMemoryRelationalStorage::deleteRows(const std::string &table,
                                      const std::optional<Predicate> &where) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end())
    return Result<size_t>::err(Status::NotFound("Unknown table: " + table));

  auto &rows = it->second.rows;
  const auto &schema = it->second.schema;

  if (!where) {
    size_t cnt = rows.size();
    rows.clear();
    return Result<size_t>::ok(cnt);
  }

  std::vector<Row> kept;
  kept.reserve(rows.size());
  size_t removed = 0;
  for (const auto &r : rows) {
    if (evalPredicate(schema, r, *where))
      ++removed;
    else
      kept.push_back(r.clone());
  }
  rows.swap(kept);
  return Result<size_t>::ok(removed);
}

Status InMemoryRelationalStorage::updateRows(
    const std::string &table,
    const std::unordered_map<std::string, std::unique_ptr<Value>> &assignments,
    const std::optional<Predicate> &where) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end())
    return Status::NotFound("Unknown table: " + table);

  auto &tableData = it->second;
  const auto &schema = tableData.schema;

  // Validate assignment columns exist
  for (const auto &kv : assignments) {
    const std::string &colName = kv.first;
    size_t idx = schema.findColumn(colName);
    if (idx == TableSchema::npos)
      return Status::InvalidArgument("Unknown assignment column: " + colName);
  }

  // Work on a copy for atomicity
  auto newRows = tableData.rows; // deep rows

  for (auto &r : newRows) {
    if (where && !evalPredicate(schema, r, *where))
      continue;
    // Apply each assignment
    for (const auto &kv : assignments) {
      const std::string &colName = kv.first;
      size_t idx = schema.findColumn(colName);
      // idx exists due to earlier validation
      // Clone the value for deep set
      std::unique_ptr<Value> v = kv.second ? kv.second->clone() : nullptr;
      r.set(idx, std::move(v));
    }
    // Validate the updated row against schema
    if (auto err = SchemaValidator::validateRow(schema, r); !err.empty()) {
      return Status::InvalidArgument(err);
    }
  }

  // Enforce uniqueness constraints after updates
  if (auto err = SchemaValidator::validateUnique(schema, newRows);
      !err.empty()) {
    return Status::FailedPrecondition(err);
  }

  // Commit
  tableData.rows.swap(newRows);
  return Status::OK();
}

Status InMemoryRelationalStorage::truncateTable(const std::string &table) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = tables_.find(table);
  if (it == tables_.end())
    return Status::NotFound("Unknown table: " + table);
  it->second.rows.clear();
  return Status::OK();
}

} // namespace kadedb
