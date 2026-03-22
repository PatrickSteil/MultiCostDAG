#include "cmdparser.hpp"
#include "graph.hpp"
#include "sweep_dag.hpp"
#include <iostream>

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "input_graph",
                                   "Input graph in DIMACs format.");
  parser.set_optional<bool>("s", "show_stats", false, "Show statistics.");
};

int main(int argc, char *argv[]) {
  cli::Parser parser(argc, argv, "MultiCostDAG");
  configure_parser(parser);
  parser.run_and_exit_if_error();

  const std::string input = parser.get<std::string>("i");
  const bool showStats = parser.get<bool>("s");

  Graph graph;
  graph.readCustomDimacsGraph(input);

  // new_id maps old id to new id
  std::vector<Vertex> new_id = graph.reorderByRank();

  if (showStats)
    graph.showStats();

  Weight coeffs[8] = {
      Weight(1, 0, 0, 0), // 0: pure time
      Weight(0, 0, 1, 0), // 1: pure cost
      Weight(0, 0, 0, 1), // 2: pure risk
      Weight(0, 1, 0, 0), // 3: minimise transfers only
      Weight(1, 3, 1, 0), // 4: penalise transfers heavily
      Weight(1, 1, 1, 1), // 5: all equal
      Weight(2, 0, 1, 3), // 6: time + risk focus
      Weight(1, 2, 3, 2), // 7: cost + transfers focus
  };

  Vertex source = new_id[0];

  SweepDAG algo(graph);
  algo.run(source, coeffs);

  for (int c = 0; c < NUM_COEFFS; ++c) {
    algo.printPath(c, new_id[10]);
  }

  return 0;
}
