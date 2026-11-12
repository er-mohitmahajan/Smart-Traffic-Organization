#ifndef TRAFFIC_QUEUE_H
#define TRAFFIC_QUEUE_H

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

namespace traffic {

// Linked FIFO queue: enqueue/dequeue/front are O(1); traversal is O(n).
template <typename T>
class Queue {
private:
    struct Node {
        explicit Node(const T& item) : value(item) {}
        T value;
        std::unique_ptr<Node> next;
    };

    std::unique_ptr<Node> head_;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Queue&& other) noexcept
        : head_(std::move(other.head_)), tail_(other.tail_), size_(other.size_) {
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            head_ = std::move(other.head_);
            tail_ = other.tail_;
            size_ = other.size_;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void enqueue(const T& item) {
        auto node = std::make_unique<Node>(item);
        Node* newTail = node.get();
        if (tail_ == nullptr) {
            head_ = std::move(node);
        } else {
            tail_->next = std::move(node);
        }
        tail_ = newTail;
        ++size_;
    }

    // O(1) priority insertion is used only for emergency vehicles.
    void enqueueFront(const T& item) {
        auto node = std::make_unique<Node>(item);
        if (head_ == nullptr) {
            tail_ = node.get();
        } else {
            node->next = std::move(head_);
        }
        head_ = std::move(node);
        ++size_;
    }

    bool dequeue(T& result) {
        if (head_ == nullptr) {
            return false;
        }
        result = head_->value;
        head_ = std::move(head_->next);
        --size_;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        return true;
    }

    T* front() { return head_ == nullptr ? nullptr : &head_->value; }
    const T* front() const { return head_ == nullptr ? nullptr : &head_->value; }
    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

    void clear() {
        head_.reset();
        tail_ = nullptr;
        size_ = 0;
    }

    template <typename Function>
    void forEach(Function&& function) const {
        const Node* node = head_.get();
        std::size_t index = 0;
        while (node != nullptr) {
            std::invoke(function, node->value, index++);
            node = node->next.get();
        }
    }
};

}  // namespace traffic

#endif
