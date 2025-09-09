#pragma once

/**
 * @file predicate_builder.h
 * @brief Convenience helpers for building composite Predicate/DocPredicate
 * trees.
 *
 * This header provides small helper functions for composing relational
 * `Predicate` and document `DocPredicate` trees. Since these predicate nodes
 * carry `std::unique_ptr<Value>` payloads (move-only), it is recommended to:
 *  - Build vectors explicitly when constructing AND/OR nodes, then move them in
 *  - Or use the provided variadic And/Or overloads which build the vectors
 *    internally and accept move-constructed children
 *
 * Example (relational):
 *
 * ```cpp
 * using namespace kadedb;
 * auto p = And(
 *   cmp("age", Predicate::Op::Ge, ValueFactory::createInteger(30)),
 *   Or(
 *     cmp("name", Predicate::Op::Eq, ValueFactory::createString("Ada")),
 *     Not(cmp("age", Predicate::Op::Lt, ValueFactory::createInteger(40)))
 *   )
 * );
 * std::optional<Predicate> where; where.emplace(std::move(p));
 * auto res = rs.select("person", {"name"}, where);
 * ```
 *
 * Example (document):
 *
 * ```cpp
 * auto p = And(
 *   dcmp("n", DocPredicate::Op::Ge, ValueFactory::createInteger(30)),
 *   Or(
 *     dcmp("k", DocPredicate::Op::Eq, ValueFactory::createString("Ada")),
 *     Not(dcmp("n", DocPredicate::Op::Lt, ValueFactory::createInteger(40)))
 *   )
 * );
 * std::optional<DocPredicate> where; where.emplace(std::move(p));
 * auto res = ds.query("cfg", {"k"}, where);
 * ```
 */

/**
 * @defgroup PredicateBuilder Predicate Builder Helpers
 * @brief Convenience helpers for composing `Predicate` and `DocPredicate`
 * trees.
 *
 * These helpers simplify building composite logical expressions for both the
 * relational and document storage APIs. Functions in this group are header-only
 * and designed for developer ergonomics in tests and examples.
 */

#include "kadedb/storage.h"

#include <memory>
#include <utility>
#include <vector>

namespace kadedb {

/// @addtogroup PredicateBuilder
/// @{

// Convenience helpers for building Predicate trees (relational)
/// Build a comparison node for a relational column.
inline Predicate cmp(const std::string &col, Predicate::Op op,
                     std::unique_ptr<Value> rhs) {
  Predicate p;
  p.kind = Predicate::Kind::Comparison;
  p.column = col;
  p.op = op;
  p.rhs = std::move(rhs);
  return p;
}

/// Build an AND node from an explicit vector of child predicates.
inline Predicate And(std::vector<Predicate> cs) { // explicit vector form
  Predicate p;
  p.kind = Predicate::Kind::And;
  p.children = std::move(cs);
  return p;
}

/// Build an OR node from an explicit vector of child predicates.
inline Predicate Or(std::vector<Predicate> cs) { // explicit vector form
  Predicate p;
  p.kind = Predicate::Kind::Or;
  p.children = std::move(cs);
  return p;
}

/// Variadic AND (relational). Accepts move-constructed children and builds the
/// vector internally.
template <typename... Ps> inline Predicate And(Ps &&...ps) {
  std::vector<Predicate> cs;
  cs.reserve(sizeof...(Ps));
  (cs.emplace_back(std::forward<Ps>(ps)), ...);
  return And(std::move(cs));
}

template <typename... Ps>
/// Variadic OR (relational). Accepts move-constructed children and builds the
/// vector internally.
inline Predicate Or(Ps &&...ps) {
  std::vector<Predicate> cs;
  cs.reserve(sizeof...(Ps));
  (cs.emplace_back(std::forward<Ps>(ps)), ...);
  return Or(std::move(cs));
}

/// Build a NOT node for relational predicates.
inline Predicate Not(Predicate c) {
  Predicate p;
  p.kind = Predicate::Kind::Not;
  p.children.push_back(std::move(c));
  return p;
}

// Convenience helpers for building DocPredicate trees (document)
/// Build a comparison node for a document field.
inline DocPredicate dcmp(const std::string &field, DocPredicate::Op op,
                         std::unique_ptr<Value> rhs) {
  DocPredicate p;
  p.kind = DocPredicate::Kind::Comparison;
  p.field = field;
  p.op = op;
  p.rhs = std::move(rhs);
  return p;
}

/// Build an AND node from an explicit vector of document child predicates.
inline DocPredicate And(std::vector<DocPredicate> cs) { // explicit vector form
  DocPredicate p;
  p.kind = DocPredicate::Kind::And;
  p.children = std::move(cs);
  return p;
}

/// Build an OR node from an explicit vector of document child predicates.
inline DocPredicate Or(std::vector<DocPredicate> cs) { // explicit vector form
  DocPredicate p;
  p.kind = DocPredicate::Kind::Or;
  p.children = std::move(cs);
  return p;
}

/// Build a NOT node for document predicates.
inline DocPredicate Not(DocPredicate c) {
  DocPredicate p;
  p.kind = DocPredicate::Kind::Not;
  p.children.push_back(std::move(c));
  return p;
}

// Variadic helpers (document)
/// Variadic AND (document). Accepts move-constructed children and builds the
/// vector internally.
template <typename... Ps> inline DocPredicate And(Ps &&...ps) {
  std::vector<DocPredicate> cs;
  cs.reserve(sizeof...(Ps));
  (cs.emplace_back(std::forward<Ps>(ps)), ...);
  return And(std::move(cs));
}

template <typename... Ps>
/// Variadic OR (document). Accepts move-constructed children and builds the
/// vector internally.
inline DocPredicate Or(Ps &&...ps) {
  std::vector<DocPredicate> cs;
  cs.reserve(sizeof...(Ps));
  (cs.emplace_back(std::forward<Ps>(ps)), ...);
  return Or(std::move(cs));
}

/// @}

} // namespace kadedb
