#include "graph.hpp"
#include "sweep_dag.hpp"
#include <iostream>

int main() {
  std::vector<Edge> edges = {
      {0, 1, Weight(3, 0, 4, 1)}, // AMS → BRU  (short hop, cheap)
      {0, 2, Weight(5, 0, 6, 2)}, // AMS → CDG
      {0, 3, Weight(9, 0, 8, 2)}, // AMS → FRA

      {1, 2, Weight(2, 1, 3, 1)}, // BRU → CDG  (quick, one transfer)
      {1, 3, Weight(6, 1, 5, 3)}, // BRU → FRA
      {2, 3, Weight(4, 0, 5, 1)}, // CDG → FRA  (direct)

      {3, 4, Weight(5, 0, 6, 1)}, // FRA → ZUR
      {3, 5, Weight(4, 0, 5, 2)}, // FRA → MUC
      {3, 6, Weight(7, 1, 9, 3)}, // FRA → VIE  (one change)
      {2, 5, Weight(8, 1, 7, 2)}, // CDG → MUC

      {4, 5, Weight(2, 0, 3, 1)}, // ZUR → MUC
      {5, 6, Weight(3, 0, 4, 1)}, // MUC → VIE

      {6, 7, Weight(10, 1, 12, 4)}, // VIE → FCO  (long, risky)
      {5, 7, Weight(9, 1, 10, 3)},  // MUC → FCO
      {4, 8, Weight(12, 1, 15, 3)}, // ZUR → MAD  (long haul)
      {3, 8, Weight(14, 0, 13, 2)}, // FRA → MAD  (direct flight)
      {3, 9, Weight(11, 0, 11, 2)}, // FRA → BCN
      {2, 9, Weight(10, 0, 9, 2)},  // CDG → BCN

      {8, 10, Weight(4, 0, 5, 1)}, // MAD → LIS
      {9, 10, Weight(6, 1, 6, 2)}, // BCN → LIS

      {7, 11, Weight(5, 0, 8, 2)},    // FCO → ATH
      {10, 11, Weight(16, 1, 18, 5)}, // LIS → ATH  (very long, risky)
  };

  Graph graph;
  graph.buildFromEdgeList(edges, 12);
  graph.showStats();

  Weight coeffs[NUM_COEFFS] = {
      Weight(1, 0, 0, 0), // 0: pure time
      Weight(0, 0, 1, 0), // 1: pure cost
      Weight(0, 0, 0, 1), // 2: pure risk
      Weight(0, 1, 0, 0), // 3: minimise transfers only
      Weight(1, 3, 1, 0), // 4: penalise transfers heavily
      Weight(1, 1, 1, 1), // 5: all equal
      Weight(2, 0, 1, 3), // 6: time + risk focus
      Weight(1, 2, 3, 2), // 7: cost + transfers focus
  };

  SweepDAG algo(graph);
  algo.run(0, coeffs);

  for (int c = 0; c < NUM_COEFFS; ++c) {
    algo.printPath(c, 10);
  }

  return 0;
}
