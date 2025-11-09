#pragma once
#include <thread>
#include <atomic>
#include "ingest_queue.hpp"
#include "batcher.h"

// Minimal ParserWorker: dequeues lines, parse minimally and append to batcher
class ParserWorker {
public:
    ParserWorker(IngestQueue& q, Batcher& b, int max_batch_dequeue = DEFAULT_MAX_BATCH_DEQUEUE);
    ~ParserWorker();

    void start();
    void stop();
private:
    void run_loop();

    IngestQueue& queue_;
    Batcher& batcher_;
    std::thread thr_;
    std::atomic<bool> running_;

    static constexpr int DEFAULT_MAX_BATCH_DEQUEUE = 1000;
    int max_batch_dequeue_;
};