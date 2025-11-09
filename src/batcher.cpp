#include "batcher.h"
#include "writer.h"
#include <chrono>
#include <iostream>

Batcher::Batcher(size_t batch_size, size_t flush_interval_ms, Writer& writer)
  : batch_size_(batch_size), flush_interval_ms_(flush_interval_ms), writer_(writer), running_(false) {
    buffer_.reserve(batch_size_ + 10);
}

Batcher::~Batcher() {
    stop();
}

void Batcher::start() {
    running_ = true;
    thr_ = std::thread([this]{ background_flush(); });
}

void Batcher::stop() {
    if (!running_) return;
    running_ = false;
    if (thr_.joinable()) thr_.join();
    // flush remaining
    std::vector<std::string> to_flush;
    {
        std::lock_guard<std::mutex> lk(mu_);
        to_flush.swap(buffer_);
    }
    if (!to_flush.empty()) {
        writer_.write_batch(to_flush);
    }
}

void Batcher::append(const std::string& rec) {
    std::lock_guard<std::mutex> lk(mu_);
    buffer_.emplace_back(rec);
    if (buffer_.size() >= batch_size_) {
        std::vector<std::string> to_flush;
        to_flush.swap(buffer_);
        lk.~lock_guard();
        writer_.write_batch(to_flush);
    }
}

void Batcher::background_flush() {
    using namespace std::chrono_literals;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(flush_interval_ms_));
        std::vector<std::string> to_flush;
        {
            std::lock_guard<std::mutex> lk(mu_);
            if (!buffer_.empty()) to_flush.swap(buffer_);
        }
        if (!to_flush.empty()) {
            writer_.write_batch(to_flush);
        }
    }
}