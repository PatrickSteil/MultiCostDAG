#pragma once

#include <cassert>
#include <random>
#include <vector>

#include "types.h"

struct Query {
  Vertex source;
  Vertex target;

  Query(Vertex s, Vertex t) : source(s), target(t) {}
};

std::vector<Query> get_queries(const std::size_t numVertices, int num_queries,
                               unsigned seed = 42) {
  assert(numVertices > 0);
  std::mt19937 rng(seed);
  std::uniform_int_distribution<Vertex> dist(0, numVertices - 1);

  std::vector<Query> queries;
  queries.reserve(num_queries);

  for (int i = 0; i < num_queries; ++i) {
    queries.emplace_back(dist(rng), dist(rng));
  }

  return queries;
}
