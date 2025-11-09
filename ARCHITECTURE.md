```markdown
# Streaming CSV/JSON → Parquet Ingest — архитетура (кратко)

Цель сервиса
- Быстро и устойчиво принимать потоки данных (CSV или JSON Lines), парсить и группировать их в батчи,
  записывать в эффективный аналитический формат (Parquet) и доставлять в объектное хранилище (S3/MinIO)
  или в аналитическую БД (например, ClickHouse).

Краткий поток данных
1. Client -> HTTP POST (chunked) или gRPC/Arrow Flight: поток CSV/JSON.
2. HTTP сервер читает чанки и кладёт сырые строки/события в thread‑safe очередь (ingest queue).
3. Worker‑пул парсит строки (simdjson / Arrow CSV), нормализует по схеме и формирует RecordBatch.
4. Batcher собирает RecordBatch до размера batch_size (или flush по таймауту) и отдаёт в ParquetWriter.
5. ParquetWriter пишет файл локально и вызывает Uploader (S3/MinIO) для отправки.
6. Метрики (Prometheus): rows/s, p95 latency, queue_length, failed_uploads, memory_usage.
7. Backpressure: при переполнении очереди сервер начинает отклонять запросы (429) или замедлять приём.

Почему это важно для заказчика
- Позволяет заменить медленные/памятоёмкие ETL (pandas) на быстрый, малопотребляющий pipeline.
- Parquet уменьшает стоимость хранения и ускоряет аналитику downstream.
- Надёжная загрузка (retries, atomic uploads) снижает риск потери данных.

Ключевые компоненты
- IngestServer (HTTP/gRPC): приём данных и simple auth.
- IngestQueue: многопоточная очередь, bounded memory.
- ParserWorkerPool: парсит CSV/JSON -> промежуточные записи.
- Batcher: собирает записи в RecordBatch.
- ParquetWriter: записывает Parquet файлы из RecordBatch.
- Uploader (S3/MinIO): загружает готовые файлы.
- MetricsExporter: Prometheus endpoint.
- (опционно) ClickHouseClient: вставка батчей напрямую.

Как смотреть UML
- Онлайн (рекомендация): https://www.planttext.com — вставь PlantUML код и нажми "Refresh".

MVP (что собрать первым)
- HTTP ingest (chunked POST) → in‑memory queue → parser → batch → Parquet file → MinIO upload.
- Benchmarks: compare with pandas/pyarrow and measure rows/s, p95, peak RSS.
- Docker Compose: сервис + MinIO.
- Small python notebook demo calling HTTP endpoint and checking uploaded Parquet.