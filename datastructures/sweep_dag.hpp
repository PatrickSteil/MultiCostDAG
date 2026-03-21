#pragma once

#include <cassert>
#include <immintrin.h>
#include <iostream>
#include <limits>
#include <vector>

#include "graph.hpp"
#include "types.h"

inline uint32_t hsum_epu32(__m128i v) {
  v = _mm_hadd_epi32(v, v);
  v = _mm_hadd_epi32(v, v);
  return _mm_cvtsi128_si32(v);
}

inline uint32_t dot_product(const Weight &w, const Weight &c) {
  __m128i vw = _mm_load_si128((const __m128i *)w.v);
  __m128i vc = _mm_load_si128((const __m128i *)c.v);
  __m128i mul = _mm_mullo_epi32(vw, vc);
  return hsum_epu32(mul);
}

class SweepDAG {
private:
  const Graph &graph;
  std::vector<uint32_t> results;
  std::vector<Vertex> parent;

public:
  SweepDAG(const Graph &graph)
      : graph(graph), results(graph.numVertices(), INF),
        parent(graph.numVertices(), noVertex) {}

  void reset() {
    std::fill(results.begin(), results.end(), INF);
    std::fill(parent.begin(), parent.end(), noVertex);
  }

  void run(const Vertex source, const Weight &coeffs) {
    assert(source < results.size());

    reset();
    results[source] = 0;

    graph.doForAllEdges(
        [&](const Vertex from, const Vertex to, const Weight &w) {
          assert(from < results.size());
          assert(to < results.size());

          uint32_t du = results[from];
          if (du == INF)
            return;

          uint32_t newCost = du + dot_product(w, coeffs);

          if (newCost < results[to]) {
            results[to] = newCost;
            parent[to] = from;
          }
        });
  }

  std::vector<Vertex> extractPath(Vertex target) const {
    std::vector<Vertex> path;

    if (results[target] == INF)
      return path; // empty = unreachable

    for (Vertex v = target; v != noVertex; v = parent[v]) {
      path.push_back(v);
    }

    std::reverse(path.begin(), path.end());
    return path;
  }

  void printPath(Vertex target) const {
    auto path = extractPath(target);

    if (path.empty()) {
      std::cout << "No path\n";
      return;
    }

    for (size_t i = 0; i < path.size(); ++i) {
      std::cout << path[i];
      if (i + 1 < path.size())
        std::cout << " -> ";
    }
    std::cout << "\n";
  }
};
