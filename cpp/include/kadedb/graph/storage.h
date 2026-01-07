#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kadedb/graph/schema.h"
#include "kadedb/result.h"
#include "kadedb/status.h"

namespace kadedb {

class GraphStorage {
public:
  virtual ~GraphStorage() = default;

  virtual Status createGraph(const std::string &graph) = 0;
  virtual Status dropGraph(const std::string &graph) = 0;
  virtual std::vector<std::string> listGraphs() const = 0;

  virtual Result<Node> getNode(const std::string &graph, NodeId id) const = 0;
  virtual Status putNode(const std::string &graph, const Node &node) = 0;
  virtual Status eraseNode(const std::string &graph, NodeId id) = 0;

  virtual Result<Edge> getEdge(const std::string &graph, EdgeId id) const = 0;
  virtual Status putEdge(const std::string &graph, const Edge &edge) = 0;
  virtual Status eraseEdge(const std::string &graph, EdgeId id) = 0;

  // Neighbor lookups
  virtual Result<std::vector<EdgeId>> edgeIdsOut(const std::string &graph,
                                                 NodeId from) const = 0;
  virtual Result<std::vector<EdgeId>> edgeIdsIn(const std::string &graph,
                                                NodeId to) const = 0;
  virtual Result<std::vector<NodeId>> neighborsOut(const std::string &graph,
                                                   NodeId from) const = 0;
  virtual Result<std::vector<NodeId>> neighborsIn(const std::string &graph,
                                                  NodeId to) const = 0;

  // Basic traversal
  virtual Result<std::vector<NodeId>>
  bfs(const std::string &graph, NodeId start, size_t maxNodes = 0) const = 0;
  virtual Result<std::vector<NodeId>>
  dfs(const std::string &graph, NodeId start, size_t maxNodes = 0) const = 0;
};

class InMemoryGraphStorage final : public GraphStorage {
public:
  InMemoryGraphStorage() = default;
  ~InMemoryGraphStorage() override = default;

  Status createGraph(const std::string &graph) override;
  Status dropGraph(const std::string &graph) override;
  std::vector<std::string> listGraphs() const override;

  Result<Node> getNode(const std::string &graph, NodeId id) const override;
  Status putNode(const std::string &graph, const Node &node) override;
  Status eraseNode(const std::string &graph, NodeId id) override;

  Result<Edge> getEdge(const std::string &graph, EdgeId id) const override;
  Status putEdge(const std::string &graph, const Edge &edge) override;
  Status eraseEdge(const std::string &graph, EdgeId id) override;

  Result<std::vector<EdgeId>> edgeIdsOut(const std::string &graph,
                                         NodeId from) const override;
  Result<std::vector<EdgeId>> edgeIdsIn(const std::string &graph,
                                        NodeId to) const override;
  Result<std::vector<NodeId>> neighborsOut(const std::string &graph,
                                           NodeId from) const override;
  Result<std::vector<NodeId>> neighborsIn(const std::string &graph,
                                          NodeId to) const override;

  Result<std::vector<NodeId>> bfs(const std::string &graph, NodeId start,
                                  size_t maxNodes) const override;
  Result<std::vector<NodeId>> dfs(const std::string &graph, NodeId start,
                                  size_t maxNodes) const override;

private:
  struct GraphData {
    std::unordered_map<NodeId, Node> nodes;
    std::unordered_map<EdgeId, Edge> edges;
    // adjacency: node -> edge ids
    AdjacencyIndex outAdj;
    AdjacencyIndex inAdj;
  };

  std::unordered_map<std::string, GraphData> graphs_;
  mutable std::mutex mtx_;
};

} // namespace kadedb
