#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kadedb/schema.h" // Document

namespace kadedb {

using NodeId = int64_t;
using EdgeId = int64_t;

// A graph Node with optional labels and arbitrary properties.
struct Node {
  NodeId id = 0;
  std::unordered_set<std::string> labels;
  Document properties;

  Node() = default;
  Node(const Node &other)
      : id(other.id), labels(other.labels),
        properties(deepCopyDocument(other.properties)) {}
  Node(Node &&) noexcept = default;
  Node &operator=(const Node &other) {
    if (this == &other)
      return *this;
    id = other.id;
    labels = other.labels;
    properties = deepCopyDocument(other.properties);
    return *this;
  }
  Node &operator=(Node &&) noexcept = default;
};

// A graph Edge connecting two Nodes.
// - type: primary relationship type (e.g. "LIKES", "PRESCRIBED")
// - labels: optional additional labels/tags
// - properties: arbitrary key/value map
struct Edge {
  EdgeId id = 0;
  NodeId from = 0;
  NodeId to = 0;
  std::string type;
  std::unordered_set<std::string> labels;
  Document properties;

  Edge() = default;
  Edge(const Edge &other)
      : id(other.id), from(other.from), to(other.to), type(other.type),
        labels(other.labels), properties(deepCopyDocument(other.properties)) {}
  Edge(Edge &&) noexcept = default;
  Edge &operator=(const Edge &other) {
    if (this == &other)
      return *this;
    id = other.id;
    from = other.from;
    to = other.to;
    type = other.type;
    labels = other.labels;
    properties = deepCopyDocument(other.properties);
    return *this;
  }
  Edge &operator=(Edge &&) noexcept = default;
};

// Simple adjacency index structures for efficient neighbor lookups.
//
// These are storage-agnostic type aliases intended for in-memory
// implementations and can be replaced by more specialized indexes later
// (CSR/COO, compressed adjacency, etc.).
using EdgeList = std::vector<EdgeId>;
using AdjacencyIndex = std::unordered_map<NodeId, EdgeList>; // node -> edge ids

} // namespace kadedb
