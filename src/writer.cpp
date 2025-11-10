#include "writer.h"
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <filesystem>

Writer::Writer(const std::string& s3_endpoint, const std::string& bucket)
  : endpoint_(s3_endpoint), bucket_(bucket) {
    // Create output directory based on environment
    std::filesystem::path output_dir;
    
    // for Docker/Linux
    const char* work_dir = std::getenv("WORK_DIR");
    if (work_dir) {
        output_dir = std::filesystem::path(work_dir) / "data" / "out";
    } else {
        // For Windows/development - use ./output
        output_dir = std::filesystem::current_path() / "output";
    }
    
    std::filesystem::create_directories(output_dir);
    output_dir_ = output_dir.string();
    
    std::cerr << "[writer] Output directory: " << output_dir_ << std::endl;
}

static std::string timestamp_filename() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto itt = system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&itt), "%Y%m%dT%H%M%S");
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    ss << "-" << ms;
    return ss.str();
}

std::string Writer::make_local_filename() {
    std::ostringstream ss;
    ss << output_dir_ << "/batch-" << timestamp_filename() << ".ndjson";
    return ss.str();
}

bool Writer::write_batch(const std::vector<std::string>& records) {
    auto path = make_local_filename();
    std::ofstream out(path, std::ios::out | std::ios::binary);
    if (!out) {
        std::cerr << "[writer] failed to open " << path << std::endl;
        return false;
    }
    for (auto &r : records) {
        out << r << "\n";
    }
    out.close();
    std::cerr << "[writer] wrote local file: " << path << " (" << records.size() << " records)\n";

    // upload via aws cli -> requires env AWS_ACCESS_KEY_ID etc.
    // using endpoint_ (e.g., http://minio:9000) and bucket_ (e.g., ingest-bucket)
    // command: aws --endpoint-url http://minio:9000 s3 cp path s3://bucket/
    std::ostringstream cmd;
    cmd << "aws --endpoint-url " << endpoint_ << " s3 cp \"" << path << "\" s3://" << bucket_ << "/";
    std::cerr << "[writer] running: " << cmd.str() << std::endl;

    int rc = std::system(cmd.str().c_str());
    if (rc != 0) {
        std::cerr << "[writer] upload failed rc=" << rc << std::endl;
        return false;
    }
    std::cerr << "[writer] uploaded to s3://" << bucket_ << std::endl;

    return true;
}