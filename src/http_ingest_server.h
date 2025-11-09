#pragma once
// Simple HTTP ingest server (uses cpp-httplib single-header)
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include "ingest_queue.hpp"

using RawLineCallback = std::function<void(const std::string&)>;

class HttpIngestServer {
public:
    HttpIngestServer(int port, IngestQueue& queue);
    ~HttpIngestServer();

    void start();
    void stop();
    void set_raw_line_callback(RawLineCallback cb);
private:
    int port_;
    IngestQueue& queue_;
    std::thread thr_;
    std::atomic<bool> running_;
    RawLineCallback raw_cb_;
    void run();
};