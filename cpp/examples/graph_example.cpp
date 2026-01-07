#include "kadedb/graph/query.h"
#include "kadedb/graph/storage.h"

#include <iostream>
#include <string>
#include <vector>

using namespace kadedb;

static void printResultSet(const ResultSet &rs) {
  for (size_t c = 0; c < rs.columnCount(); ++c) {
    if (c)
      std::cout << ", ";
    std::cout << rs.columnNames()[c];
  }
  std::cout << "\n";

  for (size_t r = 0; r < rs.rowCount(); ++r) {
    for (size_t c = 0; c < rs.columnCount(); ++c) {
      if (c)
        std::cout << ", ";
      std::cout << rs.at(r, c).toString();
    }
    std::cout << "\n";
  }
}

static void addNode(InMemoryGraphStorage &gs, const std::string &g, NodeId id) {
  Node n;
  n.id = id;
  auto st = gs.putNode(g, n);
  if (!st.ok()) {
    std::cerr << "putNode failed: " << st.message() << "\n";
  }
}

static void addEdge(InMemoryGraphStorage &gs, const std::string &g, EdgeId id,
                    NodeId from, NodeId to, const std::string &type) {
  Edge e;
  e.id = id;
  e.from = from;
  e.to = to;
  e.type = type;
  auto st = gs.putEdge(g, e);
  if (!st.ok()) {
    std::cerr << "putEdge failed: " << st.message() << "\n";
  }
}

int main() {
  InMemoryGraphStorage gs;

  auto st = gs.createGraph("g");
  if (!st.ok()) {
    std::cerr << "createGraph failed: " << st.message() << "\n";
    return 1;
  }

  addNode(gs, "g", 1);
  addNode(gs, "g", 2);
  addNode(gs, "g", 3);
  addNode(gs, "g", 4);

  addEdge(gs, "g", 10, 1, 2, "KNOWS");
  addEdge(gs, "g", 11, 2, 3, "KNOWS");
  addEdge(gs, "g", 12, 3, 4, "LIKES");

  std::cout << "=== BFS traversal (GraphStorage API) ===\n";
  {
    auto res = gs.bfs("g", 1, 0);
    if (!res.hasValue()) {
      std::cerr << "bfs failed: " << res.status().message() << "\n";
      return 1;
    }
    for (auto n : res.value())
      std::cout << n << " ";
    std::cout << "\n";
  }

  std::cout << "\n=== Graph query: TRAVERSE ===\n";
  {
    auto res = executeGraphQuery(gs, "TRAVERSE g FROM 1 BFS LIMIT 10");
    if (!res.hasValue()) {
      std::cerr << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  std::cout << "\n=== Graph query: MATCH typed relationship ===\n";
  {
    auto res =
        executeGraphQuery(gs, "MATCH g (a)-[:KNOWS]->(b) WHERE a = 2 RETURN b");
    if (!res.hasValue()) {
      std::cerr << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  std::cout << "\n=== Graph query: SHORTEST_PATH ===\n";
  {
    auto res = executeGraphQuery(gs, "SHORTEST_PATH g FROM 1 TO 4");
    if (!res.hasValue()) {
      std::cerr << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  std::cout << "\n=== Graph query: CONNECTED ===\n";
  {
    auto res = executeGraphQuery(gs, "CONNECTED g FROM 1 TO 4");
    if (!res.hasValue()) {
      std::cerr << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  return 0;
}
