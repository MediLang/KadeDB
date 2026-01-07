#include "kadedb/graph/query.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "kadedb/value.h"

namespace kadedb {
namespace {

static std::string toUpper(std::string s) {
  for (auto &c : s)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

static bool ieq(const std::string &a, const std::string &b) {
  return toUpper(a) == toUpper(b);
}

static Result<int64_t> parseInt64(const std::string &s) {
  try {
    return Result<int64_t>::ok(std::stoll(s));
  } catch (...) {
    return Result<int64_t>::err(
        Status::InvalidArgument("Invalid integer: " + s));
  }
}

static Result<ResultSet> resultNodeList(const std::vector<NodeId> &nodes) {
  ResultSet rs({"node_id"}, {ColumnType::Integer});
  for (NodeId n : nodes) {
    std::vector<std::unique_ptr<Value>> row;
    row.push_back(ValueFactory::createInteger(n));
    rs.addRow(ResultRow(std::move(row)));
  }
  return Result<ResultSet>::ok(std::move(rs));
}

static Result<ResultSet> resultPath(const std::vector<NodeId> &path) {
  ResultSet rs({"step", "node_id"}, {ColumnType::Integer, ColumnType::Integer});
  for (size_t i = 0; i < path.size(); ++i) {
    std::vector<std::unique_ptr<Value>> row;
    row.push_back(ValueFactory::createInteger(static_cast<int64_t>(i)));
    row.push_back(ValueFactory::createInteger(path[i]));
    rs.addRow(ResultRow(std::move(row)));
  }
  return Result<ResultSet>::ok(std::move(rs));
}

static Result<ResultSet> resultBool(bool v) {
  ResultSet rs({"value"}, {ColumnType::Boolean});
  std::vector<std::unique_ptr<Value>> row;
  row.push_back(ValueFactory::createBoolean(v));
  rs.addRow(ResultRow(std::move(row)));
  return Result<ResultSet>::ok(std::move(rs));
}

static Result<std::vector<NodeId>>
shortestPathUnweighted(const GraphStorage &gs, const std::string &graph,
                       NodeId start, NodeId goal) {
  if (start == goal)
    return Result<std::vector<NodeId>>::ok(std::vector<NodeId>{start});

  std::deque<NodeId> q;
  std::unordered_set<NodeId> seen;
  std::unordered_map<NodeId, NodeId> parent;

  q.push_back(start);
  seen.insert(start);

  while (!q.empty()) {
    NodeId cur = q.front();
    q.pop_front();

    auto nbrRes = gs.neighborsOut(graph, cur);
    if (!nbrRes.hasValue())
      return Result<std::vector<NodeId>>::err(nbrRes.status());

    for (NodeId nxt : nbrRes.value()) {
      if (!seen.insert(nxt).second)
        continue;
      parent[nxt] = cur;
      if (nxt == goal) {
        std::vector<NodeId> path;
        NodeId x = goal;
        while (true) {
          path.push_back(x);
          auto it = parent.find(x);
          if (it == parent.end())
            break;
          x = it->second;
          if (x == start) {
            path.push_back(x);
            break;
          }
        }
        std::reverse(path.begin(), path.end());
        return Result<std::vector<NodeId>>::ok(std::move(path));
      }
      q.push_back(nxt);
    }
  }

  return Result<std::vector<NodeId>>::ok(std::vector<NodeId>{});
}

static Result<ResultSet> execTraverse(const GraphStorage &gs,
                                      const std::vector<std::string> &toks) {
  if (toks.size() < 5)
    return Result<ResultSet>::err(
        Status::InvalidArgument("TRAVERSE syntax: TRAVERSE <graph> FROM "
                                "<start> (BFS|DFS) [LIMIT <n>]"));

  const std::string &graph = toks[1];
  if (!ieq(toks[2], "FROM"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected FROM"));

  auto sres = parseInt64(toks[3]);
  if (!sres.hasValue())
    return Result<ResultSet>::err(sres.status());
  NodeId start = sres.value();

  std::string mode = toks[4];
  size_t limit = 0;
  if (toks.size() >= 7 && ieq(toks[5], "LIMIT")) {
    auto lres = parseInt64(toks[6]);
    if (!lres.hasValue())
      return Result<ResultSet>::err(lres.status());
    if (lres.value() < 0)
      return Result<ResultSet>::err(
          Status::InvalidArgument("LIMIT must be >= 0"));
    limit = static_cast<size_t>(lres.value());
  }

  if (ieq(mode, "BFS")) {
    auto r = gs.bfs(graph, start, limit);
    if (!r.hasValue())
      return Result<ResultSet>::err(r.status());
    return resultNodeList(r.value());
  }
  if (ieq(mode, "DFS")) {
    auto r = gs.dfs(graph, start, limit);
    if (!r.hasValue())
      return Result<ResultSet>::err(r.status());
    return resultNodeList(r.value());
  }

  return Result<ResultSet>::err(Status::InvalidArgument("Expected BFS or DFS"));
}

static Result<ResultSet> execConnected(const GraphStorage &gs,
                                       const std::vector<std::string> &toks) {
  if (toks.size() < 6)
    return Result<ResultSet>::err(Status::InvalidArgument(
        "CONNECTED syntax: CONNECTED <graph> FROM <a> TO <b>"));

  const std::string &graph = toks[1];
  if (!ieq(toks[2], "FROM"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected FROM"));
  auto ares = parseInt64(toks[3]);
  if (!ares.hasValue())
    return Result<ResultSet>::err(ares.status());
  if (!ieq(toks[4], "TO"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected TO"));
  auto bres = parseInt64(toks[5]);
  if (!bres.hasValue())
    return Result<ResultSet>::err(bres.status());

  auto path = shortestPathUnweighted(gs, graph, ares.value(), bres.value());
  if (!path.hasValue())
    return Result<ResultSet>::err(path.status());
  return resultBool(!path.value().empty());
}

static Result<ResultSet>
execShortestPath(const GraphStorage &gs, const std::vector<std::string> &toks) {
  if (toks.size() < 6)
    return Result<ResultSet>::err(Status::InvalidArgument(
        "SHORTEST_PATH syntax: SHORTEST_PATH <graph> FROM <a> TO <b>"));

  const std::string &graph = toks[1];
  if (!ieq(toks[2], "FROM"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected FROM"));
  auto ares = parseInt64(toks[3]);
  if (!ares.hasValue())
    return Result<ResultSet>::err(ares.status());
  if (!ieq(toks[4], "TO"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected TO"));
  auto bres = parseInt64(toks[5]);
  if (!bres.hasValue())
    return Result<ResultSet>::err(bres.status());

  auto path = shortestPathUnweighted(gs, graph, ares.value(), bres.value());
  if (!path.hasValue())
    return Result<ResultSet>::err(path.status());
  return resultPath(path.value());
}

static std::vector<std::string> tokenize(const std::string &q) {
  std::istringstream iss(q);
  std::vector<std::string> toks;
  std::string t;
  while (iss >> t)
    toks.push_back(t);
  return toks;
}

static Result<ResultSet> execMatch(const GraphStorage &gs,
                                   const std::vector<std::string> &toks) {
  // MATCH <graph> (a)-[:TYPE]->(b) WHERE a = <id> RETURN b
  if (toks.size() < 8)
    return Result<ResultSet>::err(
        Status::InvalidArgument("MATCH syntax: MATCH <graph> (a)-[:TYPE]->(b) "
                                "WHERE a = <id> RETURN b"));

  const std::string &graph = toks[1];

  std::string pattern = toks[2];
  std::string whereTok = toks[3];
  if (!ieq(whereTok, "WHERE")) {
    // allow pattern to be split by whitespace, join until WHERE
    size_t i = 3;
    while (i < toks.size() && !ieq(toks[i], "WHERE")) {
      pattern += toks[i];
      ++i;
    }
    if (i >= toks.size())
      return Result<ResultSet>::err(Status::InvalidArgument("Expected WHERE"));
  }

  size_t whereIdx = 0;
  for (size_t i = 0; i < toks.size(); ++i) {
    if (ieq(toks[i], "WHERE")) {
      whereIdx = i;
      break;
    }
  }
  if (whereIdx == 0 || whereIdx + 4 >= toks.size())
    return Result<ResultSet>::err(
        Status::InvalidArgument("Invalid WHERE clause"));

  // Expect: WHERE a = <id> RETURN b
  if (!ieq(toks[whereIdx + 1], "a"))
    return Result<ResultSet>::err(
        Status::InvalidArgument("Expected 'a' in WHERE"));
  if (toks[whereIdx + 2] != "=")
    return Result<ResultSet>::err(
        Status::InvalidArgument("Expected '=' in WHERE"));
  auto idres = parseInt64(toks[whereIdx + 3]);
  if (!idres.hasValue())
    return Result<ResultSet>::err(idres.status());
  NodeId start = idres.value();

  size_t retIdx = whereIdx + 4;
  if (retIdx + 1 >= toks.size() || !ieq(toks[retIdx], "RETURN"))
    return Result<ResultSet>::err(Status::InvalidArgument("Expected RETURN"));
  if (!ieq(toks[retIdx + 1], "b"))
    return Result<ResultSet>::err(
        Status::InvalidArgument("Only RETURN b is supported"));

  // Parse relationship type out of pattern if present: -[:TYPE]->
  std::string relType;
  auto pos = pattern.find("[:");
  if (pos != std::string::npos) {
    auto end = pattern.find("]", pos);
    if (end != std::string::npos && end > pos + 2) {
      relType = pattern.substr(pos + 2, end - (pos + 2));
    }
  }

  auto eidsRes = gs.edgeIdsOut(graph, start);
  if (!eidsRes.hasValue())
    return Result<ResultSet>::err(eidsRes.status());

  std::vector<NodeId> out;
  for (EdgeId eid : eidsRes.value()) {
    auto er = gs.getEdge(graph, eid);
    if (!er.hasValue())
      return Result<ResultSet>::err(er.status());
    const Edge &e = er.value();
    if (!relType.empty() && !ieq(e.type, relType))
      continue;
    out.push_back(e.to);
  }

  return resultNodeList(out);
}

} // namespace

Result<ResultSet> executeGraphQuery(const GraphStorage &storage,
                                    const std::string &query) {
  auto toks = tokenize(query);
  if (toks.empty()) {
    return Result<ResultSet>::err(Status::InvalidArgument("Empty graph query"));
  }

  if (ieq(toks[0], "TRAVERSE"))
    return execTraverse(storage, toks);
  if (ieq(toks[0], "MATCH"))
    return execMatch(storage, toks);
  if (ieq(toks[0], "SHORTEST_PATH"))
    return execShortestPath(storage, toks);
  if (ieq(toks[0], "CONNECTED"))
    return execConnected(storage, toks);

  return Result<ResultSet>::err(
      Status::InvalidArgument("Unknown graph query verb: " + toks[0]));
}

} // namespace kadedb
