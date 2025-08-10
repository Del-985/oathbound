#pragma once
#include <vector>
#include <algorithm>
#include "core/rng.hpp"

namespace core {

template<typename T>
class WeightedTable {
public:
    void add(const T& t, double w) {
        if (w <= 0) return;
        items_.push_back(t);
        total_ += w;
        prefix_.push_back(total_);
    }

    const T& pick(class RNG& rng) const {
        double r = rng.f(0.0, total_);
        auto it = std::lower_bound(prefix_.begin(), prefix_.end(), r);
        size_t idx = static_cast<size_t>(std::distance(prefix_.begin(), it));
        return items_[idx];
    }

    bool empty() const { return items_.empty(); }

private:
    std::vector<T> items_;
    std::vector<double> prefix_;
    double total_ = 0.0;
};

} // namespace core
