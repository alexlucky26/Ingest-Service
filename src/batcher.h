#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>

// Batcher collects records and flushes to Writer when batch_size reached or timeout elapses.
class Writer; // forward

class Batcher {
public:
    Batcher(size_t batch_size, size_t flush_interval_ms, Writer& writer);
    ~Batcher();

    void append(const std::string& rec);
    void start();
    void stop();
private:
    void background_flush();
    size_t batch_size_;
    size_t flush_interval_ms_;
    Writer& writer_;
    std::vector<std::string> buffer_;
    std::mutex mu_;
    std::thread thr_;
    std::atomic<bool> running_;
};