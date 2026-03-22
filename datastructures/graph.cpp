#include "graph.hpp"
#include "status_log.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>

Graph::Graph() : adj(1, 0) {}

Graph::Graph(std::size_t n, std::size_t m) : adj(n + 1, 0), to(m), w(m) {}

std::size_t Graph::numVertices() const { return adj.size() - 1; }
std::size_t Graph::numEdges() const { return to.size(); }
bool Graph::isValid(Vertex v) const { return v < numVertices(); }

std::size_t Graph::beginEdge(Vertex v) const {
  assert(isValid(v));
  return adj[v];
}

std::size_t Graph::endEdge(Vertex v) const {
  assert(isValid(v));
  return adj[v + 1];
}

std::size_t Graph::degree(Vertex v) const { return endEdge(v) - beginEdge(v); }

void Graph::clear() {
  adj.assign(1, 0);
  to.clear();
  w.clear();
}

void Graph::buildFromEdgeList(std::vector<Edge> &edges, std::size_t n) {
  clear();
  std::sort(edges.begin(), edges.end());

  adj.assign(n + 1, 0);
  for (const auto &e : edges) {
    assert(e.from < n);
    assert(e.to < n);
    ++adj[e.from + 1];
  }
  for (std::size_t i = 1; i <= n; ++i)
    adj[i] += adj[i - 1];

  to.resize(edges.size());
  w.resize(edges.size());

  std::vector<std::size_t> offset = adj;
  for (const auto &e : edges) {
    std::size_t idx = offset[e.from]++;
    to[idx] = e.to;
    w[idx] = e.weight;
  }
}

/*
 * Format:
c  comment line (ignored)
p  sp <n_vertices> <n_edges> <k_weights>   (problem line)
a  <from> <to> <w0> <w1> ... <wk-1>        (arc line, 1-indexed)

// Example (k = 2):
c  Small example graph
p  sp  4  5  2
a  1  2   5  10
a  1  3   3  20
a  2  3   2   5
a  2  4  10   1
a  3  4   4   8
*/

void Graph::readCustomDimacsGraph(const std::string &filename) {
  StatusLog log("Read graph from file");

  std::ifstream file(filename);
  if (!file.is_open())
    throw std::runtime_error("Cannot open file: " + filename);

  std::size_t n = 0, m = 0, k = 0;
  bool header_seen = false;
  std::vector<Edge> edges;

  std::string line;
  std::size_t lineno = 0;

  auto parse_error = [&](const std::string &msg) -> std::runtime_error {
    return std::runtime_error(filename + ":" + std::to_string(lineno) + ": " +
                              msg);
  };

  while (std::getline(file, line)) {
    ++lineno;

    if (line.empty())
      continue;

    char type = line[0];

    if (type == 'c')
      continue;

    if (type == 'p') {
      if (header_seen)
        throw parse_error("duplicate problem line");

      std::istringstream ss(line);
      std::string tag, problem_type;
      if (!(ss >> tag >> problem_type >> n >> m >> k))
        throw parse_error("malformed problem line, expected: p sp <n> <m> <k>");
      if (problem_type != "sp")
        throw parse_error("unknown problem type '" + problem_type +
                          "', expected 'sp'");
      if (k == 0 || k > 4)
        throw parse_error("k=" + std::to_string(k) +
                          " out of range, must be 1–4");

      edges.reserve(m);
      header_seen = true;
      continue;
    }

    if (type == 'a') {
      if (!header_seen)
        throw parse_error("arc line before problem descriptor");

      std::istringstream ss(line);
      std::string tag;
      Vertex u_1, v_1;
      if (!(ss >> tag >> u_1 >> v_1))
        throw parse_error("malformed arc line, expected: a <u> <v> <w0> ...");

      if (u_1 == 0 || u_1 > n)
        throw parse_error("source vertex " + std::to_string(u_1) +
                          " out of range [1," + std::to_string(n) + "]");
      if (v_1 == 0 || v_1 > n)
        throw parse_error("target vertex " + std::to_string(v_1) +
                          " out of range [1," + std::to_string(n) + "]");

      Weight wt(0);
      for (std::size_t d = 0; d < k; ++d) {
        uint32_t val;
        if (!(ss >> val))
          throw parse_error("expected " + std::to_string(k) +
                            " weight fields, got fewer");
        wt[d] = val;
      }

      edges.push_back({u_1 - 1, v_1 - 1, wt});
      continue;
    }

    std::cerr << filename << ":" << lineno << ": warning: unknown line type '"
              << type << "', skipping\n";
  }

  if (!header_seen)
    throw std::runtime_error(filename + ": no problem line found");

  if (edges.size() != m) {
    std::cerr << filename << ": warning: declared m=" << m << " but found "
              << edges.size() << " arc lines\n";
  }

  buildFromEdgeList(edges, n);
}

std::vector<Vertex> Graph::topoSort() const {
  const std::size_t n = numVertices();

  std::vector<std::size_t> indeg(n, 0);
  for (Vertex u = 0; u < n; ++u)
    for (std::size_t i = beginEdge(u); i < endEdge(u); ++i)
      ++indeg[to[i]];

  std::queue<Vertex> q;
  for (Vertex u = 0; u < n; ++u)
    if (indeg[u] == 0)
      q.push(u);

  std::vector<Vertex> order;
  order.reserve(n);

  while (!q.empty()) {
    Vertex u = q.front();
    q.pop();
    order.push_back(u);

    for (std::size_t i = beginEdge(u); i < endEdge(u); ++i) {
      Vertex v = to[i];
      if (--indeg[v] == 0)
        q.push(v);
    }
  }

  if (order.size() != n)
    throw std::runtime_error("topoSort: graph contains a cycle — not a DAG (" +
                             std::to_string(n - order.size()) +
                             " vertices not reached)");

  return order;
}

std::vector<Vertex> Graph::reorderByRank() {
  StatusLog log("Reorder graph");
  const std::size_t n = numVertices();
  std::vector<Vertex> order = topoSort();

  std::vector<Vertex> new_id(n);
  for (Vertex rank = 0; rank < n; ++rank)
    new_id[order[rank]] = rank;

  std::vector<Edge> edges;
  edges.reserve(numEdges());
  for (Vertex old_u = 0; old_u < n; ++old_u)
    for (std::size_t i = beginEdge(old_u); i < endEdge(old_u); ++i)
      edges.push_back({new_id[old_u], new_id[to[i]], w[i]});

  buildFromEdgeList(edges, n);

  return new_id;
}

Graph Graph::reverse() const {
  Graph rev;
  rev.adj.assign(numVertices() + 1, 0);
  rev.to.resize(numEdges());
  rev.w.resize(numEdges());

  for (Vertex u = 0; u < numVertices(); ++u)
    for (std::size_t i = beginEdge(u); i < endEdge(u); ++i)
      ++rev.adj[to[i] + 1];

  for (std::size_t i = 1; i <= numVertices(); ++i)
    rev.adj[i] += rev.adj[i - 1];

  std::vector<std::size_t> offset = rev.adj;
  for (Vertex u = 0; u < numVertices(); ++u) {
    for (std::size_t i = beginEdge(u); i < endEdge(u); ++i) {
      Vertex v = to[i];
      std::size_t idx = offset[v]++;
      rev.to[idx] = u;
      rev.w[idx] = w[i];
    }
  }
  return rev;
}

void Graph::showStats() const {
  std::size_t minDeg = std::numeric_limits<std::size_t>::max();
  std::size_t maxDeg = 0;
  std::size_t total = 0;
  std::size_t isolated = 0;

  for (Vertex v = 0; v < numVertices(); ++v) {
    std::size_t d = degree(v);
    if (d == 0)
      ++isolated;
    minDeg = std::min(minDeg, d);
    maxDeg = std::max(maxDeg, d);
    total += d;
  }

  double avg = static_cast<double>(total) / numVertices();
  std::cout << "Graph Statistics:\n"
            << "\tVertices: " << numVertices() << "\n"
            << "\tEdges:    " << numEdges() << "\n"
            << "\tMinDeg:   " << minDeg << "\n"
            << "\tMaxDeg:   " << maxDeg << "\n"
            << "\tAvgDeg:   " << avg << "\n"
            << "\tIsolated: " << isolated << "\n";
}
