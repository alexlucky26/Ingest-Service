#pragma once
// Minimal C++ interfaces corresponding to UML class diagram
// (header-only, illustrative; implement later)

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace ingest {

struct ConvertOptions {
  int64_t batch_size = 64 * 1024;       // rows
  int64_t block_size_bytes = 1 << 20;   // 1 MiB
  int threads = -1;                     // -1 = auto
  std::string compression = "SNAPPY";
};

struct Record {
  // simple key/value-like representation for MVP
  std::vector<std::pair<std::string, std::string>> fields;
};

struct RecordBatch {
  std::vector<Record> records;
  int64_t num_rows() const { return static_cast<int64_t>(records.size()); }
};

class IngestQueue {
public:
  IngestQueue(size_t max_items);
  bool enqueue(std::string line);               // returns false if queue full
  std::vector<std::string> dequeue_bulk(size_t n);
  size_t size() const;
  // ...
private:
  // implementation detail
};

class ParserWorker {
public:
  ParserWorker(IngestQueue& q);
  void run();                                   // loop: dequeue -> parse -> send to batcher
  Record parse_line(const std::string& line);   // uses simdjson or Arrow CSV
};

class Batcher {
public:
  Batcher(size_t batch_size);
  void append(const Record& r);
  bool should_flush() const;
  RecordBatch flush();
private:
  size_t batch_size_;
  RecordBatch current_;
};

class ParquetWriter {
public:
  ParquetWriter(const ConvertOptions& opt);
  // write RecordBatch to local parquet file, return filepath
  std::string write(const RecordBatch& batch);
};

class S3Uploader {
public:
  S3Uploader(const std::string& endpoint, const std::string& bucket);
  bool upload(const std::string& local_path, const std::string& key);
};

class MetricsExporter {
public:
  void start(int port); // start /metrics endpoint
  void inc_rows(int64_t n);
  void observe_latency_ms(double ms);
  void set_queue_length(size_t n);
};

} // namespace ingest