#include <iostream>
#include <cstdlib>
#include <string>
#include <csignal>

#include "ingest_queue.hpp"
#include "http_ingest_server.h"
#include "parser_worker.h"
#include "batcher.h"
#include "writer.h"

int main(int argc, char** argv) {
    // Read configuration from env
    int http_port = 8080;
    std::string s3_endpoint = std::getenv("S3_ENDPOINT") ? std::getenv("S3_ENDPOINT") : "http://localhost:9000"; // http://minio:9000
    std::string s3_bucket = std::getenv("S3_BUCKET") ? std::getenv("S3_BUCKET") : "ingest-bucket";
    size_t batch_size = std::getenv("BATCH_SIZE") ? std::stoul(std::getenv("BATCH_SIZE")) : 1000;
    size_t queue_capacity = std::getenv("QUEUE_CAPACITY") ? std::stoul(std::getenv("QUEUE_CAPACITY")) : 100000;
    size_t flush_interval_ms = std::getenv("FLUSH_INTERVAL_MS") ? std::stoul(std::getenv("FLUSH_INTERVAL_MS")) : 5000;

    std::cerr << "[main] S3 endpoint: " << s3_endpoint << " bucket: " << s3_bucket << "\n";
    std::cerr << "[main] batch_size=" << batch_size << " queue_capacity=" << queue_capacity << "\n";

    IngestQueue queue(queue_capacity);
    Writer writer(s3_endpoint, s3_bucket);
    Batcher batcher(batch_size, flush_interval_ms, writer);
    ParserWorker parser(queue, batcher);

    HttpIngestServer server(http_port, queue);
    /*
    // Logging raw lines for debugging
    server.set_raw_line_callback([](const std::string& line) {
        std::cout << "[RAW] " << line << std::endl;
    });
     */

    // Start components
    batcher.start();
    parser.start();
    server.start();

    // Handle SIGINT to gracefully stop
    std::atomic<bool> running{true};
    std::signal(SIGINT, [](int){ std::cerr << "[main] SIGINT received\n"; std::exit(0); });

    // Blocking main thread; server currently runs inside its own thread (httplib::Server::listen blocks)
    // For MVP we'll wait indefinitely
    while (true) std::this_thread::sleep_for(std::chrono::seconds(60));

    // cleanup (unreachable in current MVP)
    server.stop();
    parser.stop();
    batcher.stop();
    return 0;
}