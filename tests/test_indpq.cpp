#include "IndPQ.h"
#include <algorithm>
#include <climits>
#include <map>
#include <random>
#include <type_traits>

void check(bool condition) {
    if (!condition) throw std::runtime_error("Test failed");
}
template<class F> void throws(F action) {
    bool caught = false;
    try { action(); } catch (const std::runtime_error&) { caught = true; }
    check(caught);
}
using Model = std::map<std::string, int>;
void verify(const IndPQ& q, const Model& model) {
    check(q.size() == static_cast<int>(model.size()));
    check(q.isEmpty() == model.empty());
    if (model.empty()) { throws([&] { q.getMin(); }); return; }
    int minimum = INT_MAX;
    for (const auto& entry : model) minimum = std::min(minimum, entry.second);
    const auto found = model.find(q.getMin());
    check(found != model.end() && found->second == minimum);
}
void drain(IndPQ& q, Model& model) {
    while (!model.empty()) {
        verify(q, model);
        const auto id = q.getMin();
        check(q.deleteMin() == id);
        model.erase(id);
    }
    verify(q, model);
}
int main() {
    static_assert(!std::is_copy_constructible_v<IndPQ>);
    static_assert(!std::is_move_constructible_v<IndPQ>);
    static_assert(std::is_same_v<decltype(std::declval<const IndPQ&>().getMin()),
                                 const std::string&>);
    IndPQ q;
    throws([&] { q.deleteMin(); });
    throws([&] { q.remove("missing"); });
    throws([&] { q.updatePriority("missing", 1); });
    q.insert("", INT_MAX);
    throws([&] { q.insert("", 5); });
    q.insert("low", INT_MIN);
    check(q.deleteMin() == "low");
    q.updatePriority("", INT_MIN);
    check(q.deleteMin().empty());

    // Find a wraparound collision chain at the original table capacity.
    std::vector<std::string> collisions;
    for (int i = 0; collisions.size() < 4; ++i) {
        auto id = "collision-" + std::to_string(i);
        if (std::hash<std::string>{}(id) % 100 == 99) collisions.push_back(id);
    }
    for (int i = 0; i < 4; ++i) q.insert(collisions[i], i);
    q.remove(collisions[0]);
    q.remove(collisions[2]);
    q.updatePriority(collisions[3], -1);
    check(q.deleteMin() == collisions[3]);
    throws([&] { q.insert(collisions[1], 10); });
    q.insert(collisions[0], -2);
    check(q.deleteMin() == collisions[0]);
    check(q.deleteMin() == collisions[1]);

    Model model;
    // Repeated growth, priority changes, arbitrary removals, and full draining.
    for (int i = 0; i < 5000; ++i) {
        const auto id = "bulk-" + std::to_string(i);
        q.insert(id, 5000 - i); model[id] = 5000 - i;
    }
    for (int i = 0; i < 5000; i += 3) {
        const auto id = "bulk-" + std::to_string(i);
        q.remove(id); model.erase(id);
    }
    for (int i = 1; i < 5000; i += 3) {
        const auto id = "bulk-" + std::to_string(i);
        q.updatePriority(id, -i); model[id] = -i;
    }
    drain(q, model);
    q.clear();

    // Independent reference model; ties may return any minimum-priority ID.
    for (unsigned seed : {1u, 225u, 2026u}) {
        std::mt19937 rng(seed);
        for (int step = 0; step < 30000; ++step) {
            const auto id = "task-" + std::to_string(rng() % 500);
            const int priority = static_cast<int>(rng() % 101) - 50;
            const auto found = model.find(id);
            switch (rng() % 6) {
            case 0:
            case 1:
                if (found == model.end()) { q.insert(id, priority); model[id] = priority; }
                else throws([&] { q.insert(id, priority); });
                break;
            case 2:
                if (found != model.end()) { q.updatePriority(id, priority); model[id] = priority; }
                else throws([&] { q.updatePriority(id, priority); });
                break;
            case 3:
                if (found != model.end()) { q.remove(id); model.erase(id); }
                else throws([&] { q.remove(id); });
                break;
            case 4:
                if (!model.empty()) {
                    const auto minimum = q.getMin();
                    check(q.deleteMin() == minimum); model.erase(minimum);
                } else throws([&] { q.deleteMin(); });
                break;
            default: break;
            }
            verify(q, model);
            if (step % 3000 == 2999) { q.clear(); model.clear(); verify(q, model); }
        }
        drain(q, model);
    }
    std::cout << "All tests passed (including 90,000 randomized operations).\n";
}
