#include "kadedb/query_executor.h"

#include "kadedb/predicate_builder.h"
#include "kadedb/schema.h"
#include "kadedb/value.h"

#include <unordered_map>

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

    // Comparison: expect Identifier on one side and Literal on the other
    const Expression *L = be->getLeft();
    const Expression *R = be->getRight();

    const IdentifierExpression *id =
        dynamic_cast<const IdentifierExpression *>(L);
    const LiteralExpression *lit = dynamic_cast<const LiteralExpression *>(R);

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

  return storage_.select(select.getTableName(), cols, predRes.value());
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
    auto upd = storage_.updateRows(table, assigns, predRes.value());
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
    auto upd = storage_.updateRowsWith(table, updater, predRes.value());
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

  auto res = storage_.deleteRows(table, predRes.value());
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
