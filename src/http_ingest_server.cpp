#include "http_ingest_server.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <../include/httplib.h>

HttpIngestServer::HttpIngestServer(int port, IngestQueue& queue)
  : port_(port), queue_(queue), running_(false) {}

HttpIngestServer::~HttpIngestServer() {
    stop();
}

/// @brief for real-time processing of raw lines as they arrive. The callback for debugging, logging, metrics, etc.
/// @param cb  callback function taking a single string argument (the raw line)
void HttpIngestServer::set_raw_line_callback(RawLineCallback cb) {
    raw_cb_ = std::move(cb);
}

void HttpIngestServer::start() {
    running_ = true;
    thr_ = std::thread([this]{ run(); }); // single thread created because svr.listen is blocking call
}

void HttpIngestServer::stop() {
    if (!running_) return;
    running_ = false;
    // stopping httplib server: create server and call stop() from thread
    // For simplicity, we exit process on stop in MVP, or rely on join
    if (thr_.joinable()) thr_.join();
}

void HttpIngestServer::run() {
    httplib::Server svr;
    svr.Post("/ingest", [this](const httplib::Request& req, httplib::Response& res) {
        // Optional: simple token-based auth (for MVP, hardcoded)
        // std::string auth = req.get_header_value("Authorization");
        // if (auth != "Bearer SECRET_TOKEN") {
        //     res.status = 401;
        //     res.set_content("Unauthorized\n", "text/plain");
        //     return;
        // }

        // Simple: req.body contains full request. For very large streaming bodies consider
        // using alternative servers with streaming handlers. This is MVP.
        const std::string& body = req.body;
        size_t pos = 0;
        size_t accepted = 0;
        size_t failed = 0;

        while (pos < body.size()) {
            size_t nl = body.find('\n', pos);
            std::string line;
            if (nl == std::string::npos) {
                line = body.substr(pos);
                pos = body.size();
            } else {
                line = body.substr(pos, nl - pos);
                pos = nl + 1;
            }
            if (line.empty()) continue;
            // Try enqueue
            if (!queue_.try_enqueue(line)) {
                ++failed;
                // If full, we immediately respond with 429 (Too Many Requests)
                res.status = 429;
                res.set_content("queue full, try later\n", "text/plain");
                return;
            } else {
                ++accepted;
                if (raw_cb_) raw_cb_(line); // optional immediate callback (we still rely on queue)
            }
        }
        std::ostringstream oss;
        oss << "accepted=" << accepted << " failed=" << failed << "\n";
        res.status = 200;
        res.set_content(oss.str(), "text/plain");
    });

    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.status = 200;
        res.set_content("ok\n", "text/plain");
    });

    svr.Get("/metrics", [this](const httplib::Request&, httplib::Response& res) {
        std::ostringstream oss;
        oss << "ingest_queue_size " << queue_.size() << "\n";
        oss << "ingest_queue_enqueue_failures " << queue_.enqueue_failures() << "\n";
        res.status = 200;
        res.set_content(oss.str(), "text/plain");
    });

    std::cerr << "[ingest] HTTP server listening on 0.0.0.0:" << port_ << std::endl;
    svr.listen("0.0.0.0", port_);
}