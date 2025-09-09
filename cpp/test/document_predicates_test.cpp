#include "kadedb/predicate_builder.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <optional>
#include <vector>

using namespace kadedb;

int main() {
  InMemoryDocumentStorage ds;

  // Create collection lazily by put(), with no schema for flexibility
  {
    Document d;
    d["k"] = ValueFactory::createString("Ada");
    d["n"] = ValueFactory::createInteger(36);
    assert(ds.put("cfg", "a", d).ok());
  }
  {
    Document d;
    d["k"] = ValueFactory::createString("Grace");
    d["n"] = ValueFactory::createInteger(41);
    assert(ds.put("cfg", "b", d).ok());
  }
  {
    Document d;
    d["k"] = ValueFactory::createString("Bob");
    d["n"] = ValueFactory::createInteger(29);
    assert(ds.put("cfg", "c", d).ok());
  }

  // AND: n >= 30 AND n <= 40 -> Ada only
  {
    std::vector<DocPredicate> cs;
    cs.emplace_back(
        dcmp("n", DocPredicate::Op::Ge, ValueFactory::createInteger(30)));
    cs.emplace_back(
        dcmp("n", DocPredicate::Op::Le, ValueFactory::createInteger(40)));
    std::optional<DocPredicate> where;
    where.emplace(And(std::move(cs)));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 1);
  }

  // OR: k == Ada OR n < 30 -> Ada and Bob (2)
  {
    std::vector<DocPredicate> cs;
    cs.emplace_back(
        dcmp("k", DocPredicate::Op::Eq, ValueFactory::createString("Ada")));
    cs.emplace_back(
        dcmp("n", DocPredicate::Op::Lt, ValueFactory::createInteger(30)));
    std::optional<DocPredicate> where;
    where.emplace(Or(std::move(cs)));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 2);
  }

  // NOT: NOT(k == Ada) -> Grace and Bob (2)
  {
    std::optional<DocPredicate> where;
    where.emplace(Not(
        dcmp("k", DocPredicate::Op::Eq, ValueFactory::createString("Ada"))));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 2);
  }

  // Corner cases: empty children semantics
  // AND with zero children -> true (neutral), returns all (3)
  {
    DocPredicate p;
    p.kind = DocPredicate::Kind::And;
    std::optional<DocPredicate> where;
    where.emplace(std::move(p));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 3);
  }
  // OR with zero children -> false (neutral), returns 0
  {
    DocPredicate p;
    p.kind = DocPredicate::Kind::Or;
    std::optional<DocPredicate> where;
    where.emplace(std::move(p));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 0);
  }
  // NOT with zero children -> false, returns 0
  {
    DocPredicate p;
    p.kind = DocPredicate::Kind::Not;
    std::optional<DocPredicate> where;
    where.emplace(std::move(p));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 0);
  }

  // Nested: A AND (B OR NOT C)
  // A: n >= 30
  // B: k == Ada
  // C: n < 40
  // Expect: Ada and Grace (2)
  {
    DocPredicate A =
        dcmp("n", DocPredicate::Op::Ge, ValueFactory::createInteger(30));
    DocPredicate B =
        dcmp("k", DocPredicate::Op::Eq, ValueFactory::createString("Ada"));
    DocPredicate C =
        dcmp("n", DocPredicate::Op::Lt, ValueFactory::createInteger(40));
    std::vector<DocPredicate> inner;
    inner.emplace_back(std::move(B));
    inner.emplace_back(Not(std::move(C)));
    DocPredicate innerOr = Or(std::move(inner));
    std::vector<DocPredicate> outer;
    outer.emplace_back(std::move(A));
    outer.emplace_back(std::move(innerOr));
    std::optional<DocPredicate> where;
    where.emplace(And(std::move(outer)));
    auto res = ds.query("cfg", {"k"}, where);
    assert(res.hasValue());
    const auto &vec = res.value();
    assert(vec.size() == 2);
  }

  return 0;
}
