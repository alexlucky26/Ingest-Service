#pragma once
// Bounded thread-safe queue for ingest lines
// Simple header-only implementation

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <atomic>

class IngestQueue {
public:
    explicit IngestQueue(size_t max_items = 100000)
      : max_items_(max_items), enqueue_failures_(0) {}

    // try to enqueue without blocking; return false if full
    bool try_enqueue(std::string item) {
        std::unique_lock<std::mutex> lk(mu_);
        if (queue_.size() >= max_items_) {
            ++enqueue_failures_;
            return false;
        }
        queue_.emplace_back(std::move(item));
        lk.unlock();
        cv_.notify_one();
        return true;
    }

    // dequeue up to max_items; wait up to timeout when empty
    template<typename Rep, typename Period>
    std::vector<std::string> dequeue_bulk(size_t max_items, std::chrono::duration<Rep, Period> timeout) {
        std::vector<std::string> out;
        std::unique_lock<std::mutex> lk(mu_);
        if (!cv_.wait_for(lk, timeout, [this]{ return !queue_.empty(); })) {
            return out;
        }
        size_t n = std::min(max_items, queue_.size());
        out.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            out.emplace_back(std::move(queue_.front()));
            queue_.pop_front();
        }
        lk.unlock();
        return out;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return queue_.size();
    }

    size_t max_items() const { return max_items_; }
    size_t enqueue_failures() const { return enqueue_failures_.load(); }

private:
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::deque<std::string> queue_;
    size_t max_items_;
    std::atomic<size_t> enqueue_failures_;
};