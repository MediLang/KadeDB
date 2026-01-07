#include "kadedb/graph/query.h"
#include "kadedb/graph/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <iostream>

using namespace kadedb;

static void addNode(InMemoryGraphStorage &gs, const std::string &g, NodeId id) {
  Node n;
  n.id = id;
  auto st = gs.putNode(g, n);
  assert(st.ok());
}

static void addEdge(InMemoryGraphStorage &gs, const std::string &g, EdgeId id,
                    NodeId from, NodeId to, const std::string &type) {
  Edge e;
  e.id = id;
  e.from = from;
  e.to = to;
  e.type = type;
  auto st = gs.putEdge(g, e);
  assert(st.ok());
}

int main() {
  std::cout << "graph_test starting\n";

  InMemoryGraphStorage gs;
  {
    auto st = gs.createGraph("g");
    assert(st.ok());
    auto st2 = gs.createGraph("g");
    assert(!st2.ok());
  }

  // CRUD: nodes
  {
    addNode(gs, "g", 1);
    addNode(gs, "g", 2);

    auto n1 = gs.getNode("g", 1);
    assert(n1.hasValue());
    assert(n1.value().id == 1);

    auto miss = gs.getNode("g", 999);
    assert(!miss.hasValue());
    assert(miss.status().code() == StatusCode::NotFound);

    auto stErase = gs.eraseNode("g", 2);
    assert(stErase.ok());
    auto n2 = gs.getNode("g", 2);
    assert(!n2.hasValue());
  }

  // Re-add nodes and edges for adjacency/traversal/query tests
  {
    addNode(gs, "g", 2);
    addNode(gs, "g", 3);
    addNode(gs, "g", 4);

    addEdge(gs, "g", 10, 1, 2, "KNOWS");
    addEdge(gs, "g", 11, 2, 3, "KNOWS");
    addEdge(gs, "g", 12, 3, 4, "LIKES");

    // Neighbor lookups
    auto nbr1 = gs.neighborsOut("g", 1);
    assert(nbr1.hasValue());
    assert(nbr1.value().size() == 1);
    assert(nbr1.value()[0] == 2);

    auto in3 = gs.neighborsIn("g", 3);
    assert(in3.hasValue());
    assert(in3.value().size() == 1);
    assert(in3.value()[0] == 2);

    // edgeIdsOut
    auto eout2 = gs.edgeIdsOut("g", 2);
    assert(eout2.hasValue());
    assert(eout2.value().size() == 1);
    assert(eout2.value()[0] == 11);

    // BFS/DFS traversal
    auto bfs = gs.bfs("g", 1, 0);
    assert(bfs.hasValue());
    assert(bfs.value().size() >= 1);
    assert(bfs.value()[0] == 1);

    auto dfs = gs.dfs("g", 1, 0);
    assert(dfs.hasValue());
    assert(dfs.value().size() >= 1);
    assert(dfs.value()[0] == 1);

    // Graph query: TRAVERSE
    {
      auto res = executeGraphQuery(gs, "TRAVERSE g FROM 1 BFS LIMIT 4");
      assert(res.hasValue());
      const auto &rs = res.value();
      assert(rs.columnNames()[0] == "node_id");
      assert(rs.rowCount() >= 1);
      assert(rs.at(0, 0).asInt() == 1);
    }

    // Graph query: MATCH typed relationship
    {
      auto res = executeGraphQuery(
          gs, "MATCH g (a)-[:KNOWS]->(b) WHERE a = 2 RETURN b");
      assert(res.hasValue());
      const auto &rs = res.value();
      assert(rs.rowCount() == 1);
      assert(rs.at(0, 0).asInt() == 3);
    }

    // Graph query: SHORTEST_PATH
    {
      auto res = executeGraphQuery(gs, "SHORTEST_PATH g FROM 1 TO 4");
      assert(res.hasValue());
      const auto &rs = res.value();
      assert(rs.columnCount() == 2);
      assert(rs.columnNames()[0] == "step");
      assert(rs.columnNames()[1] == "node_id");
      // Should end at node 4
      assert(rs.rowCount() >= 1);
      assert(rs.at(rs.rowCount() - 1, 1).asInt() == 4);
    }

    // Graph query: CONNECTED
    {
      auto res = executeGraphQuery(gs, "CONNECTED g FROM 1 TO 4");
      assert(res.hasValue());
      const auto &rs = res.value();
      assert(rs.columnNames()[0] == "value");
      assert(rs.at(0, 0).asBool() == true);
    }
  }

  std::cout << "graph_test passed\n";
  return 0;
}
