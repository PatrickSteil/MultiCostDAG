#pragma once

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <numeric>

typedef uint32_t Vertex;
typedef std::size_t Index;

typedef uint32_t Time;

struct alignas(16) Weight {
  uint32_t v[4];

  Weight(uint32_t w = 0) : v{w, w, w, w} {}

  Weight(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
      : v{a0, a1, a2, a3} {}

  uint32_t &operator[](std::size_t i) { return v[i]; }
  const uint32_t &operator[](std::size_t i) const { return v[i]; }
};

constexpr Time INF = std::numeric_limits<Time>::max() / 2;

constexpr Vertex noVertex = std::numeric_limits<Vertex>::max();
constexpr Index noIndex = std::numeric_limits<Index>::max();
