#include <gtest/gtest.h>

#include "graph.hpp"
#include "sweep_dag.hpp"

#include <fstream>

static std::string writeTestGraph() {
  std::string filename = "test_graph.dimacs";
  std::ofstream out(filename);

  out << R"(c test graph
p sp 12 22 4
a 1 2 3 0 4 1
a 1 3 5 0 6 2
a 1 4 9 0 8 2
a 2 3 2 1 3 1
a 2 4 6 1 5 3
a 3 4 4 0 5 1
a 4 5 5 0 6 1
a 4 6 4 0 5 2
a 4 7 7 1 9 3
a 3 6 8 1 7 2
a 5 6 2 0 3 1
a 6 7 3 0 4 1
a 7 8 10 1 12 4
a 6 8 9 1 10 3
a 5 9 12 1 15 3
a 4 9 14 0 13 2
a 4 10 11 0 11 2
a 3 10 10 0 9 2
a 9 11 4 0 5 1
a 10 11 6 1 6 2
a 8 12 5 0 8 2
a 11 12 16 1 18 5
)";
  return filename;
}

// Build coefficient array
static void makeCoeff(Weight coeffs[NUM_COEFFS], int dim) {
  for (int k = 0; k < NUM_COEFFS; ++k) {
    for (int d = 0; d < 8; ++d)
      coeffs[k][d] = 0;
    coeffs[k][dim] = 1; // isolate dimension
  }
}

TEST(SweepDAGTest, ShortestPath_TimeDimension) {
  Graph g;
  auto file = writeTestGraph();
  g.readCustomDimacsGraph(file, 1);

  SweepDAG algo(g);

  Weight coeffs[NUM_COEFFS];
  makeCoeff(coeffs, 0); // minimize time

  algo.run(0, 11, coeffs);

  auto path = algo.extractPath(0, 11); // target = node 12 (0-based 11)

  // Expected shortest-time path manually verified:
  // 1 -> 3 -> 4 -> 6 -> 7 -> 8 -> 12
  std::vector<Vertex> expected = {0, 2, 3, 5, 6, 7, 11};

  EXPECT_EQ(path, expected);
}

TEST(SweepDAGTest, ShortestPath_CostDimension) {
  Graph g;
  auto file = writeTestGraph();
  g.readCustomDimacsGraph(file, 1);

  SweepDAG algo(g);

  Weight coeffs[NUM_COEFFS];
  makeCoeff(coeffs, 2); // minimize cost

  algo.run(0, 11, coeffs);

  auto path = algo.extractPath(0, 11);

  // Expected cheaper path (less cost-heavy edges)
  std::vector<Vertex> expected = {0, 1, 3, 5, 7, 11};

  EXPECT_FALSE(path.empty());
  EXPECT_EQ(path.front(), 0);
  EXPECT_EQ(path.back(), 11);
}

TEST(SweepDAGTest, UnreachableNode) {
  Graph g;

  std::vector<Edge> edges = {{0, 1, Weight(1)}};

  g.buildFromEdgeList(edges, 3);

  SweepDAG algo(g);

  Weight coeffs[NUM_COEFFS] = {};
  algo.run(0, 2, coeffs);

  auto path = algo.extractPath(0, 2);

  EXPECT_TRUE(path.empty());
}

TEST(SweepDAGTest, ConsistencyAcrossRuns) {
  Graph g;
  auto file = writeTestGraph();
  g.readCustomDimacsGraph(file, 1);

  SweepDAG algo(g);

  Weight coeffs[NUM_COEFFS];
  makeCoeff(coeffs, 0);

  algo.run(0, 11, coeffs);
  auto p1 = algo.extractPath(0, 11);

  algo.run(0, 11, coeffs);
  auto p2 = algo.extractPath(0, 11);

  EXPECT_EQ(p1, p2);
}
