#include "cmdparser.hpp"
#include "graph.hpp"
#include "queries.hpp"
#include "sweep_dag.hpp"

#include <iostream>
#include <thread>

void configure_parser(cli::Parser &parser) {
  parser.set_required<std::string>("i", "input_graph",
                                   "Input graph in DIMACs format.");
  parser.set_optional<bool>("s", "show_stats", false, "Show statistics.");
  parser.set_optional<int>("n", "num_queries", 1000,
                           "Run this many random vertex-to-vertex queries.");
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
  int numQueries = parser.get<int>("n");
  int numThreads = parser.get<int>("t");

  numQueries = std::max(numQueries, 1);

  numThreads = std::max(numThreads, 1);
  numThreads = std::min(numThreads, (int)std::thread::hardware_concurrency());

  Graph graph;
  graph.readCustomDimacsGraph(input, numThreads);

  std::vector<Vertex> old_to_new_mapping = graph.reorderByRank(numThreads);

  if (showStats)
    graph.showStats();

  Weight coeffs[8] = {
      Weight(1, 0, 0, 0, 0, 0, 0, 0), Weight(0, 1, 0, 0, 0, 0, 0, 0),
      Weight(0, 0, 1, 0, 0, 0, 0, 0), Weight(0, 0, 0, 1, 0, 0, 0, 0),
      Weight(0, 0, 0, 0, 1, 0, 0, 0), Weight(0, 0, 0, 0, 0, 1, 0, 0),
      Weight(0, 0, 0, 0, 0, 0, 1, 0), Weight(0, 0, 0, 0, 0, 0, 0, 1),
  };

  SweepDAG algo(graph);
  std::vector<Query> queries = get_queries(graph.numVertices(), numQueries);
  std::size_t counter = 0;

  auto start = std::chrono::high_resolution_clock::now();
  for (const auto &q : queries) {
    algo.run(q.source, q.target, coeffs);
    counter += algo.numFoundPaths(q.target);
  }

  auto end = std::chrono::high_resolution_clock::now();

  double total_duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  double avg_duration_ms = total_duration_ms / static_cast<double>(numQueries);

  std::cout << "Average runtime per Dijkstra query: " << avg_duration_ms
            << " ms\n";

  std::cout << "Found " << counter << " journeys of " << numQueries
            << " total queries\n";
  return 0;
}
