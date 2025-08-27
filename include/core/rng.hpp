#pragma once
#include <random>

namespace core {

struct RNG {
    std::mt19937_64 eng;
    explicit RNG(uint64_t seed = std::random_device{}()) : eng(seed) {}
    int i(int a, int b) { std::uniform_int_distribution<int> d(a,b); return d(eng); }
    double f(double a, double b) { std::uniform_real_distribution<double> d(a,b); return d(eng); }
    bool chance(double p) { return f(0.0,1.0) < p; }
};

} // namespace core