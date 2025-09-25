#include "kadedb/kadeql_ast.h"
#include <sstream>

namespace kadedb {
namespace kadeql {

std::string LiteralExpression::toString() const {
  return std::visit(
      [](const auto &value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, std::string>) {
          return "'" + value + "'";
        } else {
          return std::to_string(value);
        }
      },
      value_);
}

std::string BinaryExpression::toString() const {
  std::ostringstream oss;
  oss << "(" << left_->toString() << " " << operatorToString(operator_) << " "
      << right_->toString() << ")";
  return oss.str();
}

std::string BinaryExpression::operatorToString(Operator op) {
  switch (op) {
  case Operator::EQUALS:
    return "=";
  case Operator::NOT_EQUALS:
    return "!=";
  case Operator::LESS_THAN:
    return "<";
  case Operator::GREATER_THAN:
    return ">";
  case Operator::LESS_EQUAL:
    return "<=";
  case Operator::GREATER_EQUAL:
    return ">=";
  case Operator::AND:
    return "AND";
  case Operator::OR:
    return "OR";
  default:
    return "UNKNOWN";
  }
}

std::string SelectStatement::toString() const {
  std::ostringstream oss;
  oss << "SELECT ";

  if (columns_.empty() || (columns_.size() == 1 && columns_[0] == "*")) {
    oss << "*";
  } else {
    for (size_t i = 0; i < columns_.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << columns_[i];
    }
  }

  oss << " FROM " << table_name_;

  if (where_clause_) {
    oss << " WHERE " << where_clause_->toString();
  }

  return oss.str();
}

std::string InsertStatement::toString() const {
  std::ostringstream oss;
  oss << "INSERT INTO " << table_name_;

  if (!columns_.empty()) {
    oss << " (";
    for (size_t i = 0; i < columns_.size(); ++i) {
      if (i > 0)
        oss << ", ";
      oss << columns_[i];
    }
    oss << ")";
  }

  oss << " VALUES ";

  for (size_t row = 0; row < values_.size(); ++row) {
    if (row > 0)
      oss << ", ";
    oss << "(";
    for (size_t col = 0; col < values_[row].size(); ++col) {
      if (col > 0)
        oss << ", ";
      oss << values_[row][col]->toString();
    }
    oss << ")";
  }

  return oss.str();
}

} // namespace kadeql
} // namespace kadedb
