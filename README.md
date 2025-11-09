```markdown
# streaming-ingest-mvp

Skeleton: Streaming CSV/JSON → “Parquet” (MVP) ingestion service (Linux/Docker)

Что есть:
- HTTP ingest server (POST /ingest) — transport layer
- InMemory bounded IngestQueue (thread-safe)
- ParserWorker (consumer) — dequeues lines, does minimal normalization (CSV split or pass-through JSON)
- Batcher — собирает records и flushes when batch_size reached
- Writer/Uploader — writes batch to local file and uploads to MinIO using `aws s3 --endpoint-url`
- Simple metrics: /metrics (plain text counters)
- Docker Compose with MinIO (dev)
- Minimal dependencies (C++17, single header httplib)

Важно:
- Это MVP: для production замените Writer на real ParquetWriter (Arrow), robust S3 client (aws-sdk-cpp), add auth/TLS, Prometheus client, retries/backoff, WAL or broker (Kafka) for durability.

Quick start (Linux, Docker):
1) Clone repo
2) Start MinIO and service:
   docker-compose up --build

3) Create bucket in MinIO (console http://localhost:9001, user=minioadmin pass=minioadmin) or via mc/awscli:
   # using awscli from host (you can install awscli with pip)
   export AWS_ACCESS_KEY_ID=minioadmin
   export AWS_SECRET_ACCESS_KEY=minioadmin
   aws --endpoint-url http://localhost:9000 s3 mb s3://ingest-bucket

4) Send a CSV or JSONL file (example using curl):
   curl -v --header "Content-Type: text/csv" --data-binary @data/sample.csv http://localhost:8080/ingest

5) Check uploads in MinIO console or list via awscli:
   aws --endpoint-url http://localhost:9000 s3 ls s3://ingest-bucket

What to configure:
- Environment variables for service (docker-compose sets defaults):
  - S3_ENDPOINT (e.g., http://minio:9000)
  - S3_BUCKET (e.g., ingest-bucket)
  - AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY (for aws-cli uploader)
  - BATCH_SIZE (rows per file)
  - QUEUE_CAPACITY (max in-memory lines)

Files of interest:
- src/: core C++ code (ingest_queue, http server, parser, batcher, writer)
- docker/: Dockerfile
- docker-compose.yml: service + MinIO

Next steps (recommended):
- Replace Writer with Arrow/Parquet writer (C++ Arrow API).
- Add Parquet metadata and unique file naming (UUID + dataset + partition).
- Add robust uploader (aws-sdk-cpp) with retries and backoff.
- Add Prometheus proper client and Grafana dashboard.
- Optionally, support gRPC / Arrow Flight transport by implementing the same IngestServer interface.