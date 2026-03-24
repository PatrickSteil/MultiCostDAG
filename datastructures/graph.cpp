#include "graph.hpp"
#include "status_log.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

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

void Graph::buildFromEdgeList(std::vector<Edge> &edges, std::size_t n,
                              const int numThreads) {
  clear();
  std::sort(edges.begin(), edges.end());

  const std::size_t m = edges.size();

  std::vector<PaddedAtomic> degree(n);
  for (auto &d : degree)
    d.value.store(0, std::memory_order_relaxed);

  std::vector<std::thread> threads;

  auto worker_count = [&](std::size_t tid) {
    std::size_t l = tid * m / numThreads;
    std::size_t r = (tid + 1) * m / numThreads;

    for (std::size_t i = l; i < r; ++i) {
      degree[edges[i].from].value.fetch_add(1, std::memory_order_relaxed);
    }
  };

  for (int t = 0; t < numThreads; ++t)
    threads.emplace_back(worker_count, t);
  for (auto &th : threads)
    th.join();

  adj.resize(n + 1);
  std::size_t sum = 0;
  for (std::size_t v = 0; v < n; ++v) {
    adj[v] = sum;
    sum += degree[v].value.load(std::memory_order_relaxed);
  }
  adj[n] = sum;

  to.resize(m);
  w.resize(m);

  std::vector<PaddedAtomic> offset(n);
  for (std::size_t v = 0; v < n; ++v)
    offset[v].value.store(adj[v], std::memory_order_relaxed);

  threads.clear();

  auto worker_scatter = [&](std::size_t tid) {
    std::size_t l = tid * m / numThreads;
    std::size_t r = (tid + 1) * m / numThreads;

    for (std::size_t i = l; i < r; ++i) {
      const auto &e = edges[i];

      std::size_t idx =
          offset[e.from].value.fetch_add(1, std::memory_order_relaxed);

      to[idx] = e.to;
      w[idx] = e.weight;
    }
  };

  for (int t = 0; t < numThreads; ++t)
    threads.emplace_back(worker_scatter, t);
  for (auto &th : threads)
    th.join();
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

void Graph::readCustomDimacsGraph(const std::string &filename,
                                  const int numThreads) {
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

  buildFromEdgeList(edges, n, numThreads);
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

std::vector<Vertex> Graph::reorderByRank(const int numThreads) {
  StatusLog log("Reorder graph");

  const std::size_t n = numVertices();
  const std::size_t m = numEdges();

  std::vector<Vertex> order = topoSort();

  std::vector<Vertex> new_id(n);

  {
    std::vector<std::thread> threads;
    auto worker = [&](int tid) {
      std::size_t l = tid * n / numThreads;
      std::size_t r = (tid + 1) * n / numThreads;
      for (std::size_t i = l; i < r; ++i)
        new_id[order[i]] = i;
    };

    for (int t = 0; t < numThreads; ++t)
      threads.emplace_back(worker, t);
    for (auto &th : threads)
      th.join();
  }

  std::vector<PaddedAtomic> degree(n);
  for (auto &d : degree)
    d.value.store(0, std::memory_order_relaxed);

  {
    std::vector<std::thread> threads;

    auto worker = [&](int tid) {
      std::size_t l = tid * n / numThreads;
      std::size_t r = (tid + 1) * n / numThreads;

      for (Vertex u = l; u < r; ++u) {
        Vertex nu = new_id[u];
        for (std::size_t i = beginEdge(u); i < endEdge(u); ++i) {
          degree[nu].value.fetch_add(1, std::memory_order_relaxed);
        }
      }
    };

    for (int t = 0; t < numThreads; ++t)
      threads.emplace_back(worker, t);
    for (auto &th : threads)
      th.join();
  }

  std::vector<std::size_t> new_adj(n + 1);
  std::size_t sum = 0;
  for (std::size_t v = 0; v < n; ++v) {
    new_adj[v] = sum;
    sum += degree[v].value.load(std::memory_order_relaxed);
  }
  new_adj[n] = sum;

  std::vector<Vertex> new_to(m);
  std::vector<Weight> new_w(m);

  std::vector<PaddedAtomic> offset(n);
  for (std::size_t v = 0; v < n; ++v)
    offset[v].value.store(new_adj[v], std::memory_order_relaxed);

  {
    std::vector<std::thread> threads;

    auto worker = [&](int tid) {
      std::size_t l = tid * n / numThreads;
      std::size_t r = (tid + 1) * n / numThreads;

      for (Vertex u = l; u < r; ++u) {
        Vertex nu = new_id[u];

        for (std::size_t i = beginEdge(u); i < endEdge(u); ++i) {
          Vertex v = to[i];
          Vertex nv = new_id[v];

          std::size_t idx =
              offset[nu].value.fetch_add(1, std::memory_order_relaxed);

          new_to[idx] = nv;
          new_w[idx] = w[i];
        }
      }
    };

    for (int t = 0; t < numThreads; ++t)
      threads.emplace_back(worker, t);
    for (auto &th : threads)
      th.join();
  }

  adj = std::move(new_adj);
  to = std::move(new_to);
  w = std::move(new_w);

  return new_id;
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
