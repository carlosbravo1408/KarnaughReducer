#ifndef PRIORITY_QUEUE_HPP
#define PRIORITY_QUEUE_HPP

#include <queue>
#include <vector>
#include <functional>

template<typename T>
class PriorityQueue {
private:
    std::priority_queue<T, std::vector<T>, std::greater<T>> elements_;

public:
    PriorityQueue() = default;

    bool empty() const {
        return elements_.empty();
    }

    void push(const T& item) {
        elements_.push(item);
    }

    T pop() {
        T item = elements_.top();
        elements_.pop();
        return item;
    }

    const T& top() const {
        return elements_.top();
    }

    size_t size() const {
        return elements_.size();
    }
};

#endif // PRIORITY_QUEUE_HPP 