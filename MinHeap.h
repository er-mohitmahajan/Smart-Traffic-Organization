#ifndef TRAFFIC_MIN_HEAP_H
#define TRAFFIC_MIN_HEAP_H

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace traffic {

// Binary min-heap: top O(1), push/pop O(log n), bottom-up build O(n).
template <typename T, typename LowerPriority = std::less<T>>
class MinHeap {
private:
    std::vector<T> items_;
    LowerPriority lowerPriority_;

    void siftUp(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!lowerPriority_(items_[index], items_[parent])) {
                break;
            }
            std::swap(items_[index], items_[parent]);
            index = parent;
        }
    }

    void siftDown(std::size_t index) {
        while (true) {
            const std::size_t left = index * 2 + 1;
            const std::size_t right = left + 1;
            std::size_t best = index;
            if (left < items_.size() &&
                lowerPriority_(items_[left], items_[best])) {
                best = left;
            }
            if (right < items_.size() &&
                lowerPriority_(items_[right], items_[best])) {
                best = right;
            }
            if (best == index) {
                return;
            }
            std::swap(items_[index], items_[best]);
            index = best;
        }
    }

public:
    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }
    const T* top() const { return items_.empty() ? nullptr : &items_.front(); }

    void push(const T& value) {
        items_.push_back(value);
        siftUp(items_.size() - 1);
    }

    bool pop(T& result) {
        if (items_.empty()) {
            return false;
        }
        result = std::move(items_.front());
        if (items_.size() == 1) {
            items_.pop_back();
            return true;
        }
        items_.front() = std::move(items_.back());
        items_.pop_back();
        siftDown(0);
        return true;
    }

    void build(const std::vector<T>& values) {
        items_ = values;
        for (std::size_t i = items_.size() / 2; i > 0; --i) {
            siftDown(i - 1);
        }
    }

    void clear() { items_.clear(); }
};

}  // namespace traffic

#endif
