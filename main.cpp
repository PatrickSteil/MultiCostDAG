#include "cmdparser.hpp"
#include "graph.hpp"
#include "sweep_dag.hpp"

#include <iostream>
#include <thread>

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "input_graph",
                                   "Input graph in DIMACs format.");
  parser.set_optional<bool>("s", "show_stats", false, "Show statistics.");
  parser.set_optional<int>(
      "t", "num_threads", std::thread::hardware_concurrency(),
      "Number of threads to use while e.g. building datastructures.");
};

int main(int argc, char *argv[]) {
  cli::Parser parser(argc, argv, "MultiCostDAG");
  configure_parser(parser);
  parser.run_and_exit_if_error();

  const std::string input = parser.get<std::string>("i");
  const bool showStats = parser.get<bool>("s");
  int numThreads = parser.get<int>("t");

  numThreads = std::max(numThreads, 1);
  numThreads = std::min(numThreads, (int)std::thread::hardware_concurrency());

  Graph graph;
  graph.readCustomDimacsGraph(input, numThreads);

  std::vector<Vertex> old_to_new_mapping = graph.reorderByRank(numThreads);

  if (showStats)
    graph.showStats();

  Weight coeffs[8] = {
      Weight(1, 0, 0, 0, 0, 0, 0, 0), // 0: minimize transfers
      Weight(0, 0, 1, 0, 0, 0, 0, 0), // 1: minimise transfer duration
  };

  Vertex source = old_to_new_mapping[233823];
  Vertex target = old_to_new_mapping[753501];

  SweepDAG algo(graph);
  algo.run(source, coeffs);

  for (int c = 0; c < 2; ++c) {
    algo.printPath(c, target);
  }

  return 0;
}
