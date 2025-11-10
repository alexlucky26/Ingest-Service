#pragma once
#include <string>
#include <vector>

// Writer interface: writes a batch to local file and uploads to S3 endpoint (MinIO) using aws-cli
class Writer {
public:
    Writer(const std::string& s3_endpoint, const std::string& bucket);
    // write batch to local file and upload; returns true on success
    bool write_batch(const std::vector<std::string>& records);
private:
    std::string endpoint_;
    std::string bucket_;
    std::string output_dir_;
    std::string make_local_filename();
};