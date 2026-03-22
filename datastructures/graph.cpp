#include "graph.hpp"
#include "status_log.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <numeric>

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
