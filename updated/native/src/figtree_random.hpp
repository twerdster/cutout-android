// Updated 2026 deterministic, thread-local random choices for recovered FIGTree.
// Removes srand(time(NULL)) and process-global rand() side effects.
#pragma once
#include <random>
#include <stdexcept>
namespace cutout_figtree {
inline std::mt19937& generator() {
    static thread_local std::mt19937 engine(0);
    return engine;
}
inline void reset() { generator().seed(0); }
inline int index(int count) {
    if (count <= 0) throw std::invalid_argument("FIGTree random choice requires a positive count");
    return int(generator()() % unsigned(count));
}
}
