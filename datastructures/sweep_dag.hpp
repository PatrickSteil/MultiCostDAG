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

using D8 = FixedTag<uint32_t, 8>; // 8-lane u32 descriptor
using Vec8u = VFromD<D8>;         // corresponding vector type
using Msk8u = MFromD<D8>;         // corresponding mask type

struct Reg8 {
  HWY_ALIGN uint32_t v[8];

  Reg8() = default;

  HWY_ATTR explicit Reg8(uint32_t val) {
    D8 d;
    hn::Store(hn::Set(d, val), d, v);
  }
};

struct CoeffTable {
  HWY_ALIGN uint32_t col[4][8];

  CoeffTable() = default;

  explicit CoeffTable(const Weight coeffs[NUM_COEFFS]) {
    for (int d = 0; d < 4; ++d)
      for (int k = 0; k < NUM_COEFFS; ++k)
        col[d][k] = coeffs[k][d];
  }
};

HWY_ATTR inline Vec8u dot_product8(const Weight &w, const CoeffTable &ct) {
  const D8 d8;
  Vec8u acc = hn::Zero(d8);
  for (int dim = 0; dim < 4; ++dim) {
    Vec8u wd = hn::Set(d8, w[dim]);
    Vec8u col = hn::Load(d8, ct.col[dim]);
    acc = hn::Add(acc, hn::Mul(wd, col));
  }
  return acc;
}

class SweepDAG {
private:
  const Graph &graph;
  size_t numV;
  std::vector<Reg8> results; // results[v].v[k] = best cost to v under coeff k
  std::vector<Reg8> parent;  // parent[v].v[k]  = predecessor of v on that path

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

    const CoeffTable ct(coeffs);
    const D8 d8;
    const Vec8u vinf = hn::Set(d8, INF);

    graph.doForAllEdges(
        [&](const Vertex from, const Vertex to, const Weight &w) HWY_ATTR {
          assert(from < numV && to < numV);

          Vec8u du = hn::Load(d8, results[from].v);

          if (hn::AllTrue(d8, hn::Eq(du, vinf)))
            return;

          Vec8u cand = hn::Add(du, dot_product8(w, ct));

          Vec8u cur = hn::Load(d8, results[to].v);

          Msk8u mask = hn::Lt(cand, cur);

          if (hn::AllFalse(d8, mask))
            return;

          hn::Store(hn::IfThenElse(mask, cand, cur), d8, results[to].v);

          Vec8u old_par = hn::Load(d8, parent[to].v);
          Vec8u vfrom = hn::Set(d8, static_cast<uint32_t>(from));
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

} // namespace HWY_NAMESPACE
} // namespace hwy
HWY_AFTER_NAMESPACE();

using hwy::HWY_NAMESPACE::CoeffTable;
using hwy::HWY_NAMESPACE::Reg8;
using hwy::HWY_NAMESPACE::SweepDAG;
