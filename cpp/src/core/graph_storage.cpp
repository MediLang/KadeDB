#include "kadedb/graph/storage.h"

#include <algorithm>
#include <deque>
#include <unordered_set>

namespace kadedb {
namespace {

static Status graphNotFound(const std::string &g) {
  return Status::NotFound("Unknown graph: " + g);
}

static Result<Node> nodeNotFound(NodeId id) {
  return Result<Node>::err(Status::NotFound(
      "Unknown node: " + std::to_string(static_cast<long long>(id))));
}

static Result<Edge> edgeNotFound(EdgeId id) {
  return Result<Edge>::err(Status::NotFound(
      "Unknown edge: " + std::to_string(static_cast<long long>(id))));
}

static void eraseEdgeId(AdjacencyIndex &idx, NodeId n, EdgeId e) {
  auto it = idx.find(n);
  if (it == idx.end())
    return;
  auto &vec = it->second;
  vec.erase(std::remove(vec.begin(), vec.end(), e), vec.end());
  if (vec.empty())
    idx.erase(it);
}

} // namespace

Status InMemoryGraphStorage::createGraph(const std::string &graph) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (graphs_.find(graph) != graphs_.end())
    return Status::AlreadyExists("Graph already exists: " + graph);
  graphs_.emplace(graph, GraphData{});
  return Status::OK();
}

Status InMemoryGraphStorage::dropGraph(const std::string &graph) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = graphs_.find(graph);
  if (it == graphs_.end())
    return graphNotFound(graph);
  graphs_.erase(it);
  return Status::OK();
}

std::vector<std::string> InMemoryGraphStorage::listGraphs() const {
  std::lock_guard<std::mutex> lk(mtx_);
  std::vector<std::string> out;
  out.reserve(graphs_.size());
  for (const auto &kv : graphs_)
    out.push_back(kv.first);
  return out;
}

Result<Node> InMemoryGraphStorage::getNode(const std::string &graph,
                                           NodeId id) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<Node>::err(graphNotFound(graph));
  const auto &g = git->second;
  auto it = g.nodes.find(id);
  if (it == g.nodes.end())
    return nodeNotFound(id);
  return Result<Node>::ok(it->second);
}

Status InMemoryGraphStorage::putNode(const std::string &graph,
                                     const Node &node) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return graphNotFound(graph);
  auto &g = git->second;
  g.nodes[node.id] = node;
  return Status::OK();
}

Status InMemoryGraphStorage::eraseNode(const std::string &graph, NodeId id) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return graphNotFound(graph);
  auto &g = git->second;

  auto nit = g.nodes.find(id);
  if (nit == g.nodes.end())
    return Status::NotFound("Unknown node: " +
                            std::to_string(static_cast<long long>(id)));

  // collect edges to delete (out + in)
  std::vector<EdgeId> toErase;
  if (auto oit = g.outAdj.find(id); oit != g.outAdj.end()) {
    toErase.insert(toErase.end(), oit->second.begin(), oit->second.end());
  }
  if (auto iit = g.inAdj.find(id); iit != g.inAdj.end()) {
    toErase.insert(toErase.end(), iit->second.begin(), iit->second.end());
  }
  std::sort(toErase.begin(), toErase.end());
  toErase.erase(std::unique(toErase.begin(), toErase.end()), toErase.end());
  for (EdgeId e : toErase) {
    auto eit = g.edges.find(e);
    if (eit == g.edges.end())
      continue;
    const Edge &edge = eit->second;
    eraseEdgeId(g.outAdj, edge.from, edge.id);
    eraseEdgeId(g.inAdj, edge.to, edge.id);
    g.edges.erase(eit);
  }

  g.outAdj.erase(id);
  g.inAdj.erase(id);
  g.nodes.erase(nit);
  return Status::OK();
}

Result<Edge> InMemoryGraphStorage::getEdge(const std::string &graph,
                                           EdgeId id) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<Edge>::err(graphNotFound(graph));
  const auto &g = git->second;
  auto it = g.edges.find(id);
  if (it == g.edges.end())
    return edgeNotFound(id);
  return Result<Edge>::ok(it->second);
}

Status InMemoryGraphStorage::putEdge(const std::string &graph,
                                     const Edge &edge) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return graphNotFound(graph);
  auto &g = git->second;

  if (g.nodes.find(edge.from) == g.nodes.end() ||
      g.nodes.find(edge.to) == g.nodes.end()) {
    return Status::InvalidArgument("Edge endpoints must exist");
  }

  // If updating an existing edge, remove adjacency references first
  if (auto eit = g.edges.find(edge.id); eit != g.edges.end()) {
    const Edge &old = eit->second;
    eraseEdgeId(g.outAdj, old.from, old.id);
    eraseEdgeId(g.inAdj, old.to, old.id);
  }

  g.edges[edge.id] = edge;
  g.outAdj[edge.from].push_back(edge.id);
  g.inAdj[edge.to].push_back(edge.id);
  return Status::OK();
}

Status InMemoryGraphStorage::eraseEdge(const std::string &graph, EdgeId id) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return graphNotFound(graph);
  auto &g = git->second;

  auto eit = g.edges.find(id);
  if (eit == g.edges.end())
    return Status::NotFound("Unknown edge: " +
                            std::to_string(static_cast<long long>(id)));
  const Edge &edge = eit->second;
  eraseEdgeId(g.outAdj, edge.from, edge.id);
  eraseEdgeId(g.inAdj, edge.to, edge.id);
  g.edges.erase(eit);
  return Status::OK();
}

Result<std::vector<EdgeId>>
InMemoryGraphStorage::edgeIdsOut(const std::string &graph, NodeId from) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end()) {
    return Result<std::vector<EdgeId>>::err(graphNotFound(graph));
  }
  const auto &g = git->second;
  if (g.nodes.find(from) == g.nodes.end()) {
    return Result<std::vector<EdgeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(from))));
  }

  auto it = g.outAdj.find(from);
  if (it == g.outAdj.end()) {
    return Result<std::vector<EdgeId>>::ok(std::vector<EdgeId>{});
  }
  return Result<std::vector<EdgeId>>::ok(it->second);
}

Result<std::vector<EdgeId>>
InMemoryGraphStorage::edgeIdsIn(const std::string &graph, NodeId to) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end()) {
    return Result<std::vector<EdgeId>>::err(graphNotFound(graph));
  }
  const auto &g = git->second;
  if (g.nodes.find(to) == g.nodes.end()) {
    return Result<std::vector<EdgeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(to))));
  }

  auto it = g.inAdj.find(to);
  if (it == g.inAdj.end()) {
    return Result<std::vector<EdgeId>>::ok(std::vector<EdgeId>{});
  }
  return Result<std::vector<EdgeId>>::ok(it->second);
}

Result<std::vector<NodeId>>
InMemoryGraphStorage::neighborsOut(const std::string &graph,
                                   NodeId from) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<std::vector<NodeId>>::err(graphNotFound(graph));
  const auto &g = git->second;
  if (g.nodes.find(from) == g.nodes.end()) {
    return Result<std::vector<NodeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(from))));
  }

  std::vector<NodeId> out;
  auto it = g.outAdj.find(from);
  if (it == g.outAdj.end())
    return Result<std::vector<NodeId>>::ok(out);

  out.reserve(it->second.size());
  for (EdgeId eid : it->second) {
    auto eit = g.edges.find(eid);
    if (eit != g.edges.end())
      out.push_back(eit->second.to);
  }
  return Result<std::vector<NodeId>>::ok(std::move(out));
}

Result<std::vector<NodeId>>
InMemoryGraphStorage::neighborsIn(const std::string &graph, NodeId to) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<std::vector<NodeId>>::err(graphNotFound(graph));
  const auto &g = git->second;
  if (g.nodes.find(to) == g.nodes.end()) {
    return Result<std::vector<NodeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(to))));
  }

  std::vector<NodeId> out;
  auto it = g.inAdj.find(to);
  if (it == g.inAdj.end())
    return Result<std::vector<NodeId>>::ok(out);

  out.reserve(it->second.size());
  for (EdgeId eid : it->second) {
    auto eit = g.edges.find(eid);
    if (eit != g.edges.end())
      out.push_back(eit->second.from);
  }
  return Result<std::vector<NodeId>>::ok(std::move(out));
}

Result<std::vector<NodeId>> InMemoryGraphStorage::bfs(const std::string &graph,
                                                      NodeId start,
                                                      size_t maxNodes) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<std::vector<NodeId>>::err(graphNotFound(graph));
  const auto &g = git->second;
  if (g.nodes.find(start) == g.nodes.end()) {
    return Result<std::vector<NodeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(start))));
  }

  std::deque<NodeId> q;
  std::unordered_set<NodeId> seen;
  std::vector<NodeId> order;

  q.push_back(start);
  seen.insert(start);

  while (!q.empty()) {
    NodeId cur = q.front();
    q.pop_front();
    order.push_back(cur);
    if (maxNodes > 0 && order.size() >= maxNodes)
      break;

    auto adjIt = g.outAdj.find(cur);
    if (adjIt == g.outAdj.end())
      continue;
    for (EdgeId eid : adjIt->second) {
      auto eit = g.edges.find(eid);
      if (eit == g.edges.end())
        continue;
      NodeId nxt = eit->second.to;
      if (seen.insert(nxt).second) {
        q.push_back(nxt);
      }
    }
  }

  return Result<std::vector<NodeId>>::ok(std::move(order));
}

Result<std::vector<NodeId>> InMemoryGraphStorage::dfs(const std::string &graph,
                                                      NodeId start,
                                                      size_t maxNodes) const {
  std::lock_guard<std::mutex> lk(mtx_);
  auto git = graphs_.find(graph);
  if (git == graphs_.end())
    return Result<std::vector<NodeId>>::err(graphNotFound(graph));
  const auto &g = git->second;
  if (g.nodes.find(start) == g.nodes.end()) {
    return Result<std::vector<NodeId>>::err(Status::NotFound(
        "Unknown node: " + std::to_string(static_cast<long long>(start))));
  }

  std::vector<NodeId> stack;
  std::unordered_set<NodeId> seen;
  std::vector<NodeId> order;

  stack.push_back(start);

  while (!stack.empty()) {
    NodeId cur = stack.back();
    stack.pop_back();
    if (!seen.insert(cur).second)
      continue;
    order.push_back(cur);
    if (maxNodes > 0 && order.size() >= maxNodes)
      break;

    auto adjIt = g.outAdj.find(cur);
    if (adjIt == g.outAdj.end())
      continue;

    // push neighbors in reverse so the first neighbor appears earlier (stable)
    for (auto rit = adjIt->second.rbegin(); rit != adjIt->second.rend();
         ++rit) {
      auto eit = g.edges.find(*rit);
      if (eit == g.edges.end())
        continue;
      NodeId nxt = eit->second.to;
      if (seen.find(nxt) == seen.end())
        stack.push_back(nxt);
    }
  }

  return Result<std::vector<NodeId>>::ok(std::move(order));
}

} // namespace kadedb
