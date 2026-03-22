#pragma once

#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

#include "hwy/aligned_allocator.h"
#include "hwy/highway.h"

#include "graph.hpp"
#include "types.h"

static constexpr int NUM_COEFFS = 8;

HWY_BEFORE_NAMESPACE();
namespace hwy {
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET != HWY_SCALAR && HWY_TARGET != HWY_EMU128 &&                    \
    HWY_TARGET != HWY_SSE2 && HWY_TARGET != HWY_SSSE3 &&                       \
    HWY_TARGET != HWY_SSE4

struct Reg8 {
  HWY_ALIGN uint32_t v[8];

  Reg8() = default;

  HWY_ATTR explicit Reg8(uint32_t val) {
    const FixedTag<uint32_t, 8> d;
    hn::Store(hn::Set(d, val), d, v);
  }
};

struct CoeffTable {
  HWY_ALIGN uint32_t col[4][8]; // col[dim][k] = coeffs[k][dim]

  CoeffTable() = default;

  explicit CoeffTable(const Weight coeffs[NUM_COEFFS]) {
    for (int d = 0; d < 4; ++d)
      for (int k = 0; k < NUM_COEFFS; ++k)
        col[d][k] = coeffs[k][d];
  }
};

HWY_ATTR inline auto dot_product8(const Weight &w, const CoeffTable &ct) {
  const FixedTag<uint32_t, 8> d8;
  auto acc = hn::Zero(d8);
  for (int dim = 0; dim < 4; ++dim) {
    auto wd = hn::Set(d8, w[dim]);
    auto col = hn::Load(d8, ct.col[dim]);
    acc = hn::Add(acc, hn::Mul(wd, col));
  }
  return acc;
}

class SweepDAG {
private:
  const Graph &graph;
  size_t numV;
  std::vector<Reg8> results;
  std::vector<Reg8> parent;

public:
  SweepDAG(const Graph &graph)
      : graph(graph), numV(graph.numVertices()), results(numV, Reg8(INF)),
        parent(numV, Reg8(noVertex)) {}

  HWY_ATTR void reset() {
    std::fill(results.begin(), results.end(), Reg8(INF));
    std::fill(parent.begin(), parent.end(), Reg8(noVertex));
  }

  HWY_ATTR void run(const Vertex source, const Weight coeffs[NUM_COEFFS]) {
    assert(source < numV);
    reset();
    results[source] = Reg8(0);

    const FixedTag<uint32_t, 8> d8;
    const CoeffTable ct(coeffs);
    const auto vinf = hn::Set(d8, INF);

    graph.doForAllEdges(
        [&](const Vertex from, const Vertex to, const Weight &w) HWY_ATTR {
          assert(from < numV && to < numV);

          auto du = hn::Load(d8, results[from].v);
          if (hn::AllTrue(d8, hn::Eq(du, vinf)))
            return;

          auto cand = hn::Add(du, dot_product8(w, ct));
          auto cur = hn::Load(d8, results[to].v);
          auto mask = hn::Lt(cand, cur);

          if (hn::AllFalse(d8, mask))
            return;

          hn::Store(hn::IfThenElse(mask, cand, cur), d8, results[to].v);

          auto old_par = hn::Load(d8, parent[to].v);
          auto vfrom = hn::Set(d8, static_cast<uint32_t>(from));
          hn::Store(hn::IfThenElse(mask, vfrom, old_par), d8, parent[to].v);
        });
  }

  std::vector<Vertex> extractPath(int k, Vertex target) const {
    assert(k >= 0 && k < NUM_COEFFS && target < numV);
    std::vector<Vertex> path;
    if (results[target].v[k] == INF)
      return path;
    for (Vertex v = target; v != noVertex; v = parent[v].v[k])
      path.push_back(v);
    std::reverse(path.begin(), path.end());
    return path;
  }

  void printPath(int k, Vertex target) const {
    auto path = extractPath(k, target);
    std::cout << "[coeff " << k << " -> v" << target << "] ";
    if (path.empty()) {
      std::cout << "unreachable\n";
      return;
    }
    for (size_t i = 0; i < path.size(); ++i) {
      std::cout << path[i];
      if (i + 1 < path.size())
        std::cout << " -> ";
    }
    std::cout << "  (cost=" << results[target].v[k] << ")\n";
  }

  void printAllPaths() const {
    for (int k = 0; k < NUM_COEFFS; ++k)
      for (Vertex v = 0; v < numV; ++v)
        printPath(k, v);
  }

  uint32_t getResult(int k, Vertex v) const {
    assert(k >= 0 && k < NUM_COEFFS && v < numV);
    return results[v].v[k];
  }
};

#endif // HWY_TARGET guard

} // namespace HWY_NAMESPACE
} // namespace hwy
HWY_AFTER_NAMESPACE();

using hwy::HWY_NAMESPACE::CoeffTable;
using hwy::HWY_NAMESPACE::Reg8;
using hwy::HWY_NAMESPACE::SweepDAG;
