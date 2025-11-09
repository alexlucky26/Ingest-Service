#include "parser_worker.h"
#include <chrono>
#include <iostream>
#include <sstream>

ParserWorker::ParserWorker(IngestQueue& q, Batcher& b, int max_batch_dequeue)
  : queue_(q), batcher_(b), running_(false), max_batch_dequeue_(max_batch_dequeue) {}

ParserWorker::~ParserWorker() {
    stop();
}

void ParserWorker::start() {
    running_ = true;
    thr_ = std::thread([this]{ run_loop(); });
}

void ParserWorker::stop() {
    if (!running_) return;
    running_ = false;
    if (thr_.joinable()) thr_.join();
}

void ParserWorker::run_loop() {
    using namespace std::chrono_literals;
    while (running_) {
        // wait up to 500ms for data
        auto lines = queue_.dequeue_bulk(max_batch_dequeue_, 500ms);
        if (lines.empty()) continue;
        for (auto &ln : lines) {
            // Very simple parser:
            // If Content looks like CSV (contains comma), split and create short CSV-like record string.
            // Otherwise, treat as JSONL and pass through.
            std::string record;
            if (ln.find(',') != std::string::npos) {
                // normalize: remove carriage returns
                if (!ln.empty() && ln.back() == '\r') ln.pop_back();
                record = ln; // for MVP we keep CSV line as record
            } else {
                // JSON or other, keep as-is
                record = ln;
            }
            batcher_.append(record);
        }
    }
}