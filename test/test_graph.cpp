#include "graph.hpp"
#include <gtest/gtest.h>

TEST(GraphTest, EmptyGraph) {
  Graph g;
  EXPECT_EQ(g.numVertices(), 0);
  EXPECT_EQ(g.numEdges(), 0);
}

TEST(GraphTest, BuildSimpleGraph) {
  Graph g;

  std::vector<Edge> edges = {
      {0, 1, Weight(1)}, {1, 2, Weight(2)}, {0, 2, Weight(3)}};

  g.buildFromEdgeList(edges, 3);

  EXPECT_EQ(g.numVertices(), 3);
  EXPECT_EQ(g.numEdges(), 3);

  EXPECT_EQ(g.degree(0), 2);
  EXPECT_EQ(g.degree(1), 1);
  EXPECT_EQ(g.degree(2), 0);
}

TEST(GraphTest, EdgeIteration) {
  Graph g;

  std::vector<Edge> edges = {{0, 1, Weight(1)}, {0, 2, Weight(2)}};

  g.buildFromEdgeList(edges, 3);

  std::vector<Vertex> neighbors;
  g.relaxAllEdges(0, [&](const Vertex, const Vertex to, const Weight) {
    neighbors.push_back(to);
  });

  EXPECT_EQ(neighbors.size(), 2);
  EXPECT_TRUE((neighbors[0] == 1 && neighbors[1] == 2) ||
              (neighbors[0] == 2 && neighbors[1] == 1));
}

TEST(GraphTest, TopologicalSortValidDAG) {
  Graph g;

  std::vector<Edge> edges = {
      {0, 1, Weight(1)}, {1, 2, Weight(1)}, {0, 2, Weight(1)}};

  g.buildFromEdgeList(edges, 3);

  auto order = g.topoSort();

  EXPECT_EQ(order.size(), 3);

  // Check topological property
  std::vector<int> pos(3);
  for (int i = 0; i < 3; ++i)
    pos[order[i]] = i;

  EXPECT_LT(pos[0], pos[1]);
  EXPECT_LT(pos[1], pos[2]);
}

TEST(GraphTest, TopologicalSortCycle) {
  Graph g;

  std::vector<Edge> edges = {
      {0, 1, Weight(1)}, {1, 0, Weight(1)} // cycle
  };

  g.buildFromEdgeList(edges, 2);

  EXPECT_THROW(g.topoSort(), std::runtime_error);
}

TEST(GraphTest, ParallelBuildMatchesSequential) {
  Graph g1, g2;

  std::vector<Edge> edges = {{0, 1, Weight(1)},
                             {0, 2, Weight(2)},
                             {1, 2, Weight(3)},
                             {2, 3, Weight(4)}};

  g1.buildFromEdgeList(edges, 4);
  g2.buildFromEdgeList(edges, 4, 4);

  EXPECT_EQ(g1.numVertices(), g2.numVertices());
  EXPECT_EQ(g1.numEdges(), g2.numEdges());

  for (size_t v = 0; v < g1.numVertices(); ++v) {
    EXPECT_EQ(g1.degree(v), g2.degree(v));
  }
}
