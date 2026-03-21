#include "graph.hpp"
#include "sweep_dag.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
  std::vector<Edge> edges = {
      {0, 1, Weight(5, 0, 2, 1)},  // fast, cheap, low risk
      {0, 2, Weight(3, 1, 4, 3)},  // faster but worse in others
      {1, 2, Weight(2, 0, 1, 1)},  // good connection
      {1, 3, Weight(10, 0, 1, 0)}, // slow but reliable
      {2, 3, Weight(4, 1, 2, 2)},  // balanced
  };

  Graph graph;
  graph.buildFromEdgeList(edges, 4);
  graph.showStats();

  SweepDAG algo(graph);

  Weight coeff(0);
  coeff[0] = 1; // time
  coeff[1] = 3; // penalize transfers heavily
  coeff[2] = 1; // cost

  algo.run(0, coeff);

  for (Vertex v(0); v < graph.numVertices(); ++v) {
    algo.printPath(v);
  }
  return 0;
}
