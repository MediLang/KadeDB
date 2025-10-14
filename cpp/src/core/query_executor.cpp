#include "kadedb/query_executor.h"

#include "kadedb/predicate_builder.h"
#include "kadedb/schema.h"
#include "kadedb/value.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace kadedb {
namespace kadeql {

// ---- Internal helpers ----

std::unique_ptr<Value>
QueryExecutor::literalToValue(const LiteralExpression::Value &v) const {
  if (std::holds_alternative<std::string>(v)) {
    return ValueFactory::createString(std::get<std::string>(v));
  }
  if (std::holds_alternative<double>(v)) {
    return ValueFactory::createFloat(std::get<double>(v));
  }
  if (std::holds_alternative<int64_t>(v)) {
    return ValueFactory::createInteger(std::get<int64_t>(v));
  }
  return ValueFactory::createNull();
}

// Validate that all columns referenced in a predicate exist in the table schema
Status
QueryExecutor::validatePredicateColumns(const std::string &table,
                                        const std::optional<Predicate> &where) {
  if (!where)
    return Status::OK();

  // Probe schema via select * (as in executeInsert)
  auto probe = storage_.select(table, /*columns=*/{}, /*where=*/std::nullopt);
  if (!probe.hasValue()) {
    return probe.status();
  }
  const ResultSet &schemaView = probe.value();
  const auto &allCols = schemaView.columnNames();
  std::unordered_set<std::string> colset(allCols.begin(), allCols.end());

  // Recursive check
  std::function<Status(const Predicate &)> check =
      [&](const Predicate &p) -> Status {
    using K = Predicate::Kind;
    switch (p.kind) {
    case K::Comparison:
      if (colset.find(p.column) == colset.end())
        return Status::InvalidArgument("Unknown column in predicate: " +
                                       p.column);
      return Status::OK();
    case K::And:
    case K::Or:
      for (const auto &ch : p.children) {
        auto st = check(ch);
        if (!st.ok())
          return st;
      }
      return Status::OK();
    case K::Not:
      if (p.children.empty())
        return Status::OK();
      return check(p.children.front());
    }
    return Status::OK();
  };

  return check(*where);
}

static Predicate::Op toPredOp(BinaryExpression::Operator op) {
  using BO = BinaryExpression::Operator;
  switch (op) {
  case BO::EQUALS:
    return Predicate::Op::Eq;
  case BO::NOT_EQUALS:
    return Predicate::Op::Ne;
  case BO::LESS_THAN:
    return Predicate::Op::Lt;
  case BO::LESS_EQUAL:
    return Predicate::Op::Le;
  case BO::GREATER_THAN:
    return Predicate::Op::Gt;
  case BO::GREATER_EQUAL:
    return Predicate::Op::Ge;
  case BO::AND:
  case BO::OR:
  case BO::ADD:
  case BO::SUB:
  case BO::MUL:
  case BO::DIV:
    // Unsupported for storage comparison op mapping; should be filtered
    // earlier. Fall through to default error.
    break;
  }
  // Default: unreachable if callers validate operators before calling.
  // Choose a safe default and avoid warnings; comparisons are the only valid
  // inputs.
  return Predicate::Op::Eq;
}

// ----- Predicate simplification (MVP optimizer) -----

// Helpers to stringify Value/Predictate for dedup/sorting
static std::string valueKey(const Value &v) { return v.toString(); }

static std::string predKey(const Predicate &p);

static Predicate::Op invertOp(Predicate::Op op) {
  using PO = Predicate::Op;
  switch (op) {
  case PO::Eq:
    return PO::Ne;
  case PO::Ne:
    return PO::Eq;
  case PO::Lt:
    return PO::Ge;
  case PO::Le:
    return PO::Gt;
  case PO::Gt:
    return PO::Le;
  case PO::Ge:
    return PO::Lt;
  }
  return op;
}

// Forward decl
static Predicate simplifyPred(const Predicate &p);

// Helpers to create logical-constant predicates using empty-children semantics
// AND with zero children -> true; OR with zero children -> false.
static Predicate makeTruePred() {
  Predicate t;
  t.kind = Predicate::Kind::And;
  return t;
}
static Predicate makeFalsePred() {
  Predicate f;
  f.kind = Predicate::Kind::Or;
  return f;
}

// Normalize logical children: simplify each, flatten, sort, dedup
static void normalizeLogicalChildren(Predicate &p) {
  std::vector<Predicate> flat;
  flat.reserve(p.children.size());
  for (const auto &ch : p.children) {
    Predicate s = simplifyPred(ch);
    if (s.kind == p.kind) {
      // flatten
      for (auto &gc : s.children)
        flat.emplace_back(std::move(gc));
    } else {
      flat.emplace_back(std::move(s));
    }
  }
  // Sort by key for deterministic ordering
  std::vector<std::pair<std::string, size_t>> order;
  order.reserve(flat.size());
  for (size_t i = 0; i < flat.size(); ++i) {
    order.emplace_back(predKey(flat[i]), i);
  }
  std::sort(order.begin(), order.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  // Deduplicate by key
  std::vector<Predicate> dedup;
  dedup.reserve(order.size());
  std::string lastKey;
  bool first = true;
  for (const auto &kv : order) {
    const auto &key = kv.first;
    if (!first && key == lastKey)
      continue;
    dedup.emplace_back(std::move(flat[kv.second]));
    lastKey = key;
    first = false;
  }
  p.children = std::move(dedup);
}

static Predicate simplifyPred(const Predicate &p) {
  using K = Predicate::Kind;
  // Copy node first, then mutate
  Predicate out;
  out.kind = p.kind;
  switch (p.kind) {
  case K::Comparison: {
    out.column = p.column;
    out.op = p.op;
    out.rhs = p.rhs ? p.rhs->clone() : nullptr;
    return out;
  }
  case K::And:
  case K::Or: {
    // Build children without copying move-only members
    out.children.clear();
    out.children.reserve(p.children.size());
    for (const auto &ch : p.children) {
      out.children.emplace_back(simplifyPred(ch));
    }
    normalizeLogicalChildren(out);
    // Short-circuit identities with constants
    // True == And([]), False == Or([])
    if (out.kind == K::And) {
      // Remove true children; if any false child present, whole And is false
      std::vector<Predicate> kept;
      kept.reserve(out.children.size());
      for (auto &c : out.children) {
        if (c.kind == K::Or && c.children.empty()) {
          // false -> entire And becomes false
          return makeFalsePred();
        }
        if (c.kind == K::And && c.children.empty()) {
          // true -> skip
          continue;
        }
        kept.emplace_back(std::move(c));
      }
      out.children = std::move(kept);
      // If empty after removals -> true
      if (out.children.empty())
        return makeTruePred();
      return out;
    } else { // Or
      // Remove false children; if any true child present, whole Or is true
      std::vector<Predicate> kept;
      kept.reserve(out.children.size());
      for (auto &c : out.children) {
        if (c.kind == K::And && c.children.empty()) {
          // true -> entire Or becomes true
          return makeTruePred();
        }
        if (c.kind == K::Or && c.children.empty()) {
          // false -> skip
          continue;
        }
        kept.emplace_back(std::move(c));
      }
      out.children = std::move(kept);
      // If empty after removals -> false
      if (out.children.empty())
        return makeFalsePred();
      return out;
    }
  }
  case K::Not: {
    // Simplify child first (if any)
    if (p.children.empty()) {
      // NOT with no child stays as-is; evaluation will yield false
      return out; // empty NOT
    }
    Predicate child = simplifyPred(p.children.front());
    // Double negation
    if (child.kind == K::Not) {
      if (!child.children.empty()) {
        return simplifyPred(child.children.front());
      }
      Predicate emptyNot;
      emptyNot.kind = K::Not;
      return emptyNot; // remains NOT empty
    }
    // De Morgan for AND/OR
    if (child.kind == K::And || child.kind == K::Or) {
      Predicate dem;
      dem.kind = (child.kind == K::And) ? K::Or : K::And;
      dem.children.reserve(child.children.size());
      for (const auto &gc : child.children) {
        Predicate n;
        n.kind = K::Not;
        n.children.emplace_back(simplifyPred(gc));
        dem.children.emplace_back(std::move(n));
      }
      normalizeLogicalChildren(dem);
      return dem;
    }
    // NOT over comparison: invert operator
    if (child.kind == K::Comparison) {
      Predicate inv;
      inv.kind = K::Comparison;
      inv.column = child.column;
      inv.op = invertOp(child.op);
      inv.rhs = child.rhs ? child.rhs->clone() : nullptr;
      return inv;
    }
    // Fallback: wrap simplified child
    Predicate wrap;
    wrap.kind = K::Not;
    wrap.children.emplace_back(std::move(child));
    return wrap;
  }
  }
  return out; // unreachable
}

static std::string predKey(const Predicate &p) {
  using K = Predicate::Kind;
  switch (p.kind) {
  case K::Comparison: {
    std::string rhsStr = p.rhs ? valueKey(*p.rhs) : std::string("<null>");
    return std::string("C|") + p.column + "|" +
           std::to_string(static_cast<int>(p.op)) + "|" + rhsStr;
  }
  case K::And:
  case K::Or: {
    std::string s(1, p.kind == K::And ? 'A' : 'O');
    s += "|";
    // Children might be unsorted; produce keys and sort for canonical form
    std::vector<std::string> keys;
    keys.reserve(p.children.size());
    for (const auto &ch : p.children)
      keys.emplace_back(predKey(ch));
    std::sort(keys.begin(), keys.end());
    for (const auto &k : keys) {
      s += k;
      s += ",";
    }
    return s;
  }
  case K::Not: {
    std::string s("N|");
    if (!p.children.empty())
      s += predKey(p.children.front());
    return s;
  }
  }
  return "";
}

Result<std::optional<Predicate>>
QueryExecutor::buildPredicate(const Expression *expr) const {
  if (!expr)
    return Result<std::optional<Predicate>>::ok(std::nullopt);

  // UnaryExpression handling: NOT
  if (auto ue = dynamic_cast<const UnaryExpression *>(expr)) {
    if (ue->getOperator() == UnaryExpression::Operator::NOT) {
      auto cres = buildPredicate(ue->getOperand());
      if (!cres.hasValue())
        return Result<std::optional<Predicate>>::err(cres.status());
      auto opt = cres.takeValue();
      Predicate p;
      p.kind = Predicate::Kind::Not;
      if (opt) {
        p.children.emplace_back(std::move(*opt));
      }
      std::optional<Predicate> out;
      out.emplace(std::move(p));
      return Result<std::optional<Predicate>>::ok(std::move(out));
    }
  }

  // BinaryExpression handling: comparisons and AND/OR chains
  if (auto be = dynamic_cast<const BinaryExpression *>(expr)) {
    auto op = be->getOperator();
    if (op == BinaryExpression::Operator::AND ||
        op == BinaryExpression::Operator::OR) {
      auto lres = buildPredicate(be->getLeft());
      if (!lres.hasValue())
        return Result<std::optional<Predicate>>::err(lres.status());
      auto rres = buildPredicate(be->getRight());
      if (!rres.hasValue())
        return Result<std::optional<Predicate>>::err(rres.status());

      // Extract optionals and move their contents if present
      auto lopt = lres.takeValue();
      auto ropt = rres.takeValue();

      // If any side is missing (null), treat as neutral (AND true / OR false)
      std::vector<Predicate> kids;
      kids.reserve(2);
      if (lopt)
        kids.emplace_back(std::move(*lopt));
      if (ropt)
        kids.emplace_back(std::move(*ropt));

      Predicate p;
      p.kind = (op == BinaryExpression::Operator::AND) ? Predicate::Kind::And
                                                       : Predicate::Kind::Or;
      p.children = std::move(kids);
      std::optional<Predicate> out;
      out.emplace(std::move(p));
      return Result<std::optional<Predicate>>::ok(std::move(out));
    }

    // Comparison: prefer Identifier vs Literal; but allow Literal vs Literal
    const Expression *L = be->getLeft();
    const Expression *R = be->getRight();

    const IdentifierExpression *id =
        dynamic_cast<const IdentifierExpression *>(L);
    const LiteralExpression *lit = dynamic_cast<const LiteralExpression *>(R);

    // New: literal-vs-literal constant folding
    if (!id && dynamic_cast<const LiteralExpression *>(L) &&
        dynamic_cast<const LiteralExpression *>(R)) {
      // Evaluate as Value compare and return logical constant predicate
      const auto *lLit = static_cast<const LiteralExpression *>(L);
      const auto *rLit = static_cast<const LiteralExpression *>(R);
      std::unique_ptr<Value> LV = literalToValue(lLit->getValue());
      std::unique_ptr<Value> RV = literalToValue(rLit->getValue());
      int cmp = LV->compare(*RV);
      using BO = BinaryExpression::Operator;
      bool result = false;
      switch (op) {
      case BO::EQUALS:
        result = (cmp == 0);
        break;
      case BO::NOT_EQUALS:
        result = (cmp != 0);
        break;
      case BO::LESS_THAN:
        result = (cmp < 0);
        break;
      case BO::LESS_EQUAL:
        result = (cmp <= 0);
        break;
      case BO::GREATER_THAN:
        result = (cmp > 0);
        break;
      case BO::GREATER_EQUAL:
        result = (cmp >= 0);
        break;
      default:
        return Result<std::optional<Predicate>>::err(Status::InvalidArgument(
            "Unsupported operator for literal comparison"));
      }
      std::optional<Predicate> out;
      Predicate c = result ? makeTruePred() : makeFalsePred();
      out.emplace(std::move(c));
      return Result<std::optional<Predicate>>::ok(std::move(out));
    }

    bool reversed = false;
    if (!id || !lit) {
      // Try reversed order: literal op identifier (rewrite to identifier op
      // literal) and invert the operator accordingly.
      id = dynamic_cast<const IdentifierExpression *>(R);
      lit = dynamic_cast<const LiteralExpression *>(L);
      if (!id || !lit) {
        return Result<std::optional<Predicate>>::err(
            Status::InvalidArgument("Unsupported WHERE predicate: expected "
                                    "identifier compared to literal"));
      }
      reversed = true;
    }

    Predicate p;
    p.kind = Predicate::Kind::Comparison;
    p.column = id->getName();
    // If operands were reversed, invert the comparison operator
    using BO = BinaryExpression::Operator;
    if (reversed) {
      switch (op) {
      case BO::LESS_THAN:
        op = BO::GREATER_THAN;
        break;
      case BO::LESS_EQUAL:
        op = BO::GREATER_EQUAL;
        break;
      case BO::GREATER_THAN:
        op = BO::LESS_THAN;
        break;
      case BO::GREATER_EQUAL:
        op = BO::LESS_EQUAL;
        break;
      case BO::EQUALS:
      case BO::NOT_EQUALS:
      case BO::AND:
      case BO::OR:
      case BO::ADD:
      case BO::SUB:
      case BO::MUL:
      case BO::DIV:
        break; // no change or not expected here
      }
    }
    p.op = toPredOp(op);
    p.rhs = literalToValue(lit->getValue());
    std::optional<Predicate> out;
    out.emplace(std::move(p));
    return Result<std::optional<Predicate>>::ok(std::move(out));
  }

  // Identifier alone or literal alone is unsupported as a boolean predicate
  return Result<std::optional<Predicate>>::err(Status::InvalidArgument(
      "Unsupported WHERE predicate: expected binary expression"));
}

// ---- Public API ----

Result<ResultSet> QueryExecutor::execute(const Statement &statement) {
  switch (statement.type()) {
  case StatementType::SELECT:
    return executeSelect(static_cast<const SelectStatement &>(statement));
  case StatementType::INSERT:
    return executeInsert(static_cast<const InsertStatement &>(statement));
  case StatementType::UPDATE:
    return executeUpdate(static_cast<const UpdateStatement &>(statement));
  case StatementType::DELETE:
    return executeDelete(static_cast<const DeleteStatement &>(statement));
  }
  return Result<ResultSet>::err(
      Status::InvalidArgument("Unsupported statement type in executor"));
}

Result<ResultSet> QueryExecutor::executeSelect(const SelectStatement &select) {
  std::vector<std::string> cols;
  const auto &sc = select.getColumns();
  if (!(sc.size() == 1 && sc[0] == "*")) {
    cols = sc; // projection list
  }

  auto predRes = buildPredicate(select.getWhereClause());
  if (!predRes.hasValue())
    return Result<ResultSet>::err(predRes.status());
  // Simplify predicate before pushdown
  std::optional<Predicate> where = predRes.takeValue();
  if (where) {
    where = simplifyPred(*where);
  }
  // Validate referenced columns (clearer error vs silent mismatch)
  if (auto st = validatePredicateColumns(select.getTableName(), where);
      !st.ok()) {
    return Result<ResultSet>::err(st);
  }
  return storage_.select(select.getTableName(), cols, where);
}

Result<ResultSet> QueryExecutor::executeInsert(const InsertStatement &insert) {
  const std::string &table = insert.getTableName();

  // Discover schema via a select * to obtain column names/types
  auto probe = storage_.select(table, /*columns=*/{}, /*where=*/std::nullopt);
  if (!probe.hasValue()) {
    // If NotFound, still propagate; executor doesn't create tables.
    return Result<ResultSet>::err(probe.status());
  }
  const ResultSet &schemaView = probe.value();
  const auto &allCols = schemaView.columnNames();

  if (allCols.empty()) {
    // If table exists but has zero columns, nothing to insert; treat as error
    return Result<ResultSet>::err(
        Status::InvalidArgument("Target table has no columns"));
  }

  // Prepare column index mapping
  std::unordered_map<std::string, size_t> colIndex;
  for (size_t i = 0; i < allCols.size(); ++i)
    colIndex.emplace(allCols[i], i);

  // Determine the effective column order for provided VALUES
  std::vector<size_t> targetIdx;
  const auto &insertCols = insert.getColumns();
  if (insertCols.empty()) {
    // Implicit: VALUES cover all columns in table order
    targetIdx.resize(allCols.size());
    for (size_t i = 0; i < allCols.size(); ++i)
      targetIdx[i] = i;
  } else {
    targetIdx.reserve(insertCols.size());
    for (const auto &name : insertCols) {
      auto it = colIndex.find(name);
      if (it == colIndex.end()) {
        return Result<ResultSet>::err(Status::InvalidArgument(
            std::string("Unknown column in INSERT: ") + name));
      }
      targetIdx.push_back(it->second);
    }
  }

  // For each VALUES row, build a full Row of table width
  size_t inserted = 0;
  for (const auto &exprRow : insert.getValues()) {
    if (exprRow.size() != targetIdx.size()) {
      return Result<ResultSet>::err(Status::InvalidArgument(
          "INSERT VALUES arity does not match column list"));
    }

    Row row(allCols.size());
    // Initialize to nulls
    for (size_t c = 0; c < allCols.size(); ++c) {
      row.set(c, ValueFactory::createNull());
    }

    for (size_t j = 0; j < exprRow.size(); ++j) {
      const auto *e = exprRow[j].get();

      // Only literal expressions supported in VALUES for MVP
      const auto *lit = dynamic_cast<const LiteralExpression *>(e);
      if (!lit) {
        return Result<ResultSet>::err(Status::InvalidArgument(
            "INSERT VALUES only support literals in MVP"));
      }
      size_t idx = targetIdx[j];
      row.set(idx, literalToValue(lit->getValue()));
    }

    // Delegate to storage validation and insert
    auto st = storage_.insertRow(table, row);
    if (!st.ok()) {
      return Result<ResultSet>::err(st);
    }
    ++inserted;
  }

  // Return DML feedback: canonical 'affected' and legacy 'inserted'
  ResultSet rs({"affected", "inserted"},
               {ColumnType::Integer, ColumnType::Integer});
  std::vector<std::unique_ptr<Value>> cells;
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(inserted)));
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(inserted)));
  rs.addRow(ResultRow(std::move(cells)));
  return Result<ResultSet>::ok(std::move(rs));
}

Result<ResultSet> QueryExecutor::executeUpdate(const UpdateStatement &update) {
  const std::string &table = update.getTableName();

  // Build predicate
  auto predRes = buildPredicate(update.getWhereClause());
  if (!predRes.hasValue())
    return Result<ResultSet>::err(predRes.status());

  // Determine if all assignments are simple (literal or identifier)
  bool allSimple = true;
  for (const auto &asgn : update.getAssignments()) {
    const Expression *expr = asgn.second.get();
    if (!(dynamic_cast<const LiteralExpression *>(expr) ||
          dynamic_cast<const IdentifierExpression *>(expr))) {
      allSimple = false;
      break;
    }
  }

  // Simplify predicate before pushdown
  std::optional<Predicate> where = predRes.takeValue();
  if (where) {
    where = simplifyPred(*where);
  }
  // Validate referenced columns
  if (auto st = validatePredicateColumns(table, where); !st.ok()) {
    return Result<ResultSet>::err(st);
  }
  size_t affected = 0;
  if (allSimple) {
    // Fast path: use storage.updateRows with AssignmentValue map
    std::unordered_map<std::string, AssignmentValue> assigns;
    for (const auto &asgn : update.getAssignments()) {
      const auto &col = asgn.first;
      const Expression *expr = asgn.second.get();
      AssignmentValue av;
      if (auto lit = dynamic_cast<const LiteralExpression *>(expr)) {
        av.kind = AssignmentValue::Kind::Constant;
        av.constant = literalToValue(lit->getValue());
      } else {
        auto id = static_cast<const IdentifierExpression *>(expr);
        av.kind = AssignmentValue::Kind::ColumnRef;
        av.column_ref = id->getName();
      }
      assigns.emplace(col, std::move(av));
    }
    auto upd = storage_.updateRows(table, assigns, where);
    if (!upd.hasValue())
      return Result<ResultSet>::err(upd.status());
    affected = upd.value();
  } else {
    // Computed expressions: evaluate per row via updateRowsWith
    auto updater = [this, &update](Row &row,
                                   const TableSchema &schema) -> Status {
      for (const auto &asgn : update.getAssignments()) {
        const std::string &col = asgn.first;
        const Expression *expr = asgn.second.get();
        auto vres = evalExpr(expr, schema, row);
        if (!vres.hasValue())
          return vres.status();
        size_t idx = schema.findColumn(col);
        if (idx == TableSchema::npos)
          return Status::InvalidArgument("Unknown assignment column: " + col);
        row.set(idx, vres.takeValue());
      }
      return Status::OK();
    };
    auto upd = storage_.updateRowsWith(table, updater, where);
    if (!upd.hasValue())
      return Result<ResultSet>::err(upd.status());
    affected = upd.value();
  }

  // Return updated count: canonical 'affected' and legacy 'updated'
  ResultSet rs({"affected", "updated"},
               {ColumnType::Integer, ColumnType::Integer});
  std::vector<std::unique_ptr<Value>> cells;
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(affected)));
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(affected)));
  rs.addRow(ResultRow(std::move(cells)));
  return Result<ResultSet>::ok(std::move(rs));
}

Result<ResultSet> QueryExecutor::executeDelete(const DeleteStatement &del) {
  const std::string &table = del.getTableName();

  auto predRes = buildPredicate(del.getWhereClause());
  if (!predRes.hasValue())
    return Result<ResultSet>::err(predRes.status());

  // Simplify predicate before pushdown
  std::optional<Predicate> where = predRes.takeValue();
  if (where) {
    where = simplifyPred(*where);
  }
  // Validate referenced columns
  if (auto st = validatePredicateColumns(table, where); !st.ok()) {
    return Result<ResultSet>::err(st);
  }

  auto res = storage_.deleteRows(table, where);
  if (!res.hasValue()) {
    return Result<ResultSet>::err(res.status());
  }
  size_t deleted = res.value();

  ResultSet rs({"affected", "deleted"},
               {ColumnType::Integer, ColumnType::Integer});
  std::vector<std::unique_ptr<Value>> cells;
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(deleted)));
  cells.emplace_back(
      ValueFactory::createInteger(static_cast<int64_t>(deleted)));
  rs.addRow(ResultRow(std::move(cells)));
  return Result<ResultSet>::ok(std::move(rs));
}

Result<std::unique_ptr<Value>>
QueryExecutor::evalExpr(const Expression *expr, const TableSchema &schema,
                        const Row &row) const {
  // Unary logical NOT
  if (auto ue = dynamic_cast<const UnaryExpression *>(expr)) {
    if (ue->getOperator() == UnaryExpression::Operator::NOT) {
      auto vres = evalExpr(ue->getOperand(), schema, row);
      if (!vres.hasValue())
        return vres;
      std::unique_ptr<Value> v = vres.takeValue();
      bool b;
      try {
        b = v->asBool();
      } catch (...) {
        return Result<std::unique_ptr<Value>>::err(
            Status::InvalidArgument("NOT operand is not boolean-convertible"));
      }
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createBoolean(!b));
    }
  }

  if (auto lit = dynamic_cast<const LiteralExpression *>(expr)) {
    return Result<std::unique_ptr<Value>>::ok(literalToValue(lit->getValue()));
  }
  if (auto id = dynamic_cast<const IdentifierExpression *>(expr)) {
    size_t idx = schema.findColumn(id->getName());
    if (idx == TableSchema::npos)
      return Result<std::unique_ptr<Value>>::err(Status::InvalidArgument(
          "Unknown identifier in expression: " + id->getName()));
    const auto &v = row.values()[idx];
    return Result<std::unique_ptr<Value>>::ok(v ? v->clone()
                                                : ValueFactory::createNull());
  }
  if (auto be = dynamic_cast<const BinaryExpression *>(expr)) {
    using BO = BinaryExpression::Operator;
    auto lres = evalExpr(be->getLeft(), schema, row);
    if (!lres.hasValue())
      return lres;
    auto rres = evalExpr(be->getRight(), schema, row);
    if (!rres.hasValue())
      return rres;
    std::unique_ptr<Value> L = lres.takeValue();
    std::unique_ptr<Value> R = rres.takeValue();

    // Logical AND/OR
    if (be->getOperator() == BO::AND || be->getOperator() == BO::OR) {
      bool lb, rb;
      try {
        lb = L->asBool();
        rb = R->asBool();
      } catch (...) {
        return Result<std::unique_ptr<Value>>::err(Status::InvalidArgument(
            "AND/OR operands are not boolean-convertible"));
      }
      bool out = (be->getOperator() == BO::AND) ? (lb && rb) : (lb || rb);
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createBoolean(out));
    }

    // Comparisons: delegate to Value::compare
    if (be->getOperator() == BO::EQUALS ||
        be->getOperator() == BO::NOT_EQUALS ||
        be->getOperator() == BO::LESS_THAN ||
        be->getOperator() == BO::LESS_EQUAL ||
        be->getOperator() == BO::GREATER_THAN ||
        be->getOperator() == BO::GREATER_EQUAL) {
      int cmp = L->compare(*R);
      bool out = false;
      switch (be->getOperator()) {
      case BO::EQUALS:
        out = (cmp == 0);
        break;
      case BO::NOT_EQUALS:
        out = (cmp != 0);
        break;
      case BO::LESS_THAN:
        out = (cmp < 0);
        break;
      case BO::LESS_EQUAL:
        out = (cmp <= 0);
        break;
      case BO::GREATER_THAN:
        out = (cmp > 0);
        break;
      case BO::GREATER_EQUAL:
        out = (cmp >= 0);
        break;
      default:
        break;
      }
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createBoolean(out));
    }

    // Arithmetic and string concatenation
    // If either side is string, treat + as concatenation; others invalid
    auto isString = [](const Value &v) {
      return v.type() == ValueType::String;
    };
    auto stringify = [](const Value &v) -> std::string {
      if (v.type() == ValueType::String)
        return v.asString();
      return v.toString();
    };
    if (be->getOperator() == BO::ADD && (isString(*L) || isString(*R))) {
      std::string s = stringify(*L) + stringify(*R);
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createString(std::move(s)));
    }

    // Numeric-only arithmetic
    auto toDouble = [](const Value &v, bool &isInt, int64_t &iout, double &dout,
                       ValueType &t) -> bool {
      t = v.type();
      if (t == ValueType::Integer) {
        isInt = true;
        iout = v.asInt();
        dout = static_cast<double>(iout);
        return true;
      }
      if (t == ValueType::Float) {
        isInt = false;
        dout = v.asFloat();
        return true;
      }
      return false;
    };
    bool lIsInt = false;
    int64_t li = 0;
    double ld = 0.0;
    ValueType lt;
    if (!toDouble(*L, lIsInt, li, ld, lt))
      return Result<std::unique_ptr<Value>>::err(
          Status::InvalidArgument("Non-numeric LHS in arithmetic expression"));
    bool rIsInt = false;
    int64_t ri = 0;
    double rd = 0.0;
    ValueType rt;
    if (!toDouble(*R, rIsInt, ri, rd, rt))
      return Result<std::unique_ptr<Value>>::err(
          Status::InvalidArgument("Non-numeric RHS in arithmetic expression"));

    bool resultInt = lIsInt && rIsInt && be->getOperator() != BO::DIV;
    switch (be->getOperator()) {
    case BO::ADD:
      if (resultInt)
        return Result<std::unique_ptr<Value>>::ok(
            ValueFactory::createInteger(li + ri));
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createFloat(ld + rd));
    case BO::SUB:
      if (resultInt)
        return Result<std::unique_ptr<Value>>::ok(
            ValueFactory::createInteger(li - ri));
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createFloat(ld - rd));
    case BO::MUL:
      if (resultInt)
        return Result<std::unique_ptr<Value>>::ok(
            ValueFactory::createInteger(li * ri));
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createFloat(ld * rd));
    case BO::DIV:
      if (rd == 0.0)
        return Result<std::unique_ptr<Value>>::err(
            Status::InvalidArgument("Division by zero"));
      return Result<std::unique_ptr<Value>>::ok(
          ValueFactory::createFloat(ld / rd));
    default:
      return Result<std::unique_ptr<Value>>::err(Status::InvalidArgument(
          "Unsupported operator in computed expression"));
    }
  }
  return Result<std::unique_ptr<Value>>::err(
      Status::InvalidArgument("Unsupported expression in assignment"));
}

} // namespace kadeql
} // namespace kadedb
