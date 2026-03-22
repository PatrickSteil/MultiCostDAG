#pragma once

#include "types.h"
#include <cassert>
#include <vector>

struct Edge {
  Vertex from;
  Vertex to;
  Weight weight;

  Edge() = default;
  Edge(Vertex f, Vertex t, Time w = INF) : from(f), to(t), weight(INF) {
    weight[0] = w;
  }
  Edge(Vertex f, Vertex t, Weight w) : from(f), to(t), weight(w) {}

  bool operator<(const Edge &other) const {
    return std::tie(from, to) < std::tie(other.from, other.to);
  }
};

class Graph {
private:
  std::vector<Index> adj;
  std::vector<Vertex> to;
  std::vector<Weight> w;

public:
  Graph();
  Graph(std::size_t n, std::size_t m);

  std::size_t numVertices() const;
  std::size_t numEdges() const;
  bool isValid(Vertex v) const;

  std::size_t beginEdge(Vertex v) const;
  std::size_t endEdge(Vertex v) const;
  std::size_t degree(Vertex v) const;

  void clear();
  void buildFromEdgeList(std::vector<Edge> &edges, std::size_t n);
  void readCustomDimacsGraph(const std::string &filename);

  std::vector<Vertex> topoSort() const;
  std::vector<Vertex> reorderByRank();

  Graph reverse() const;

  void showStats() const;

  template <typename F> void doForAllEdges(F &&f) const {
    for (Vertex u = 0; u < numVertices(); ++u)
      for (std::size_t i = beginEdge(u); i < endEdge(u); ++i)
        f(u, to[i], w[i]);
  }

  template <typename F> void relaxAllEdges(Vertex u, F &&f) const {
    for (std::size_t i = beginEdge(u); i < endEdge(u); ++i)
      f(u, to[i], w[i]);
  }
};
