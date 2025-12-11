# AtlasSearch 90-Second Demo Script

This script demonstrates the key features of AtlasSearch in 90 seconds.

## Prerequisites

- Docker and Docker Compose installed
- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.14+
- Terminal with 4 tabs/windows

## Demo Flow

### Phase 1: Setup (20 seconds)

**Terminal 1: Start Infrastructure**

```bash
# Start Elasticsearch, Kafka, Redis
docker-compose up -d

# Wait for services to be ready
echo "Waiting for services to start..."
sleep 20

# Verify services
curl -s http://localhost:9200/_cluster/health | grep -q "green\|yellow" && echo "✓ Elasticsearch ready"
docker exec atlassearch-kafka-1 kafka-broker-api-versions --bootstrap-server localhost:9092 > /dev/null 2>&1 && echo "✓ Kafka ready"
docker exec atlassearch-redis-1 redis-cli ping | grep -q "PONG" && echo "✓ Redis ready"
```

**Terminal 2: Build Services**

```bash
# Build all services
mkdir -p build && cd build
cmake .. && make -j$(nproc)

echo "✓ Build complete"
```

### Phase 2: Search Service Demo (20 seconds)

**Terminal 2: Start Search Service**

```bash
# Start search service
./services/search-service/search_service &
SEARCH_PID=$!

sleep 2

# Test search endpoint
curl -s "http://localhost:8080/search?q=laptop&size=5" | jq .

# Run benchmark
./bench/bench_tool --concurrency=10 --requests=1000 --url="http://localhost:8080/search?q=test"

# Expected output:
# p50 = ~15 ms
# p90 = ~28 ms
# p99 = ~45 ms
# Throughput: ~400 req/s
```

### Phase 3: Consumer Pipeline Demo (20 seconds)

**Terminal 3: Start Consumer**

```bash
# Start consumer service
./services/consumer-service/consumer_service ../services/consumer-service/config.yml &
CONSUMER_PID=$!

sleep 2
```

**Terminal 4: Produce Events**

```bash
# Produce test event to Kafka
docker exec -i atlassearch-kafka-1 kafka-console-producer \
  --broker-list localhost:9092 \
  --topic product-events << EOF
{
  "product_id": "P123",
  "event_id": "evt-demo-001",
  "event_type": "update",
  "version": 5,
  "updated_at": "2025-12-11T01:00:00Z",
  "data": {
    "title": "Gaming Laptop Pro",
    "description": "Ultimate gaming laptop with RTX 4090",
    "price": 2499.99,
    "category": "Electronics"
  }
}
EOF

sleep 2

# Verify in Elasticsearch
echo "Checking Elasticsearch..."
curl -s http://localhost:9200/products/_doc/P123 | jq '.found, ._source.title'

# Verify in Redis
echo "Checking Redis..."
docker exec atlassearch-redis-1 redis-cli GET product:P123

# Expected: Event processed, stored in ES and Redis
```

### Phase 4: Ingestion Demo with Crash Recovery (30 seconds)

**Terminal 2: Start Ingestion Service**

```bash
# Kill search service
kill $SEARCH_PID

# Start ingestion demo
./services/ingest-demo/ingest_demo &
INGEST_PID=$!

sleep 2
```

**Terminal 4: Send Events**

```bash
# Send burst of events
./services/ingest-demo/producer_tool --rate=100 --burst=300 &
PRODUCER_PID=$!

# Wait 2 seconds, then kill the service (simulate crash)
sleep 2
echo "Simulating crash..."
kill -9 $INGEST_PID

# Wait for producer to finish
wait $PRODUCER_PID

# Check for pending batch files
ls -lh append-log/

# Expected: Some .jsonl files remain (pending batches)
```

**Terminal 2: Restart and Replay**

```bash
# Restart ingestion service
./services/ingest-demo/ingest_demo

# Expected output:
# Replaying pending batches...
# Found X pending batches
# Replaying batch: batch_YYYYMMDD_HHMMSS_N.jsonl (N events)
# Successfully replayed and deleted: batch_YYYYMMDD_HHMMSS_N.jsonl
# ...
# Replay complete

# Verify no pending files
ls append-log/

# Expected: Empty directory (all batches replayed)
```

## Complete Demo Output

### Search Service

```
========================================
AtlasSearch Benchmark Results
========================================
URL: http://localhost:8080/search?q=test
Concurrency: 10
Total Requests: 1000
Duration: 2.5s

Latency Distribution:
  p50  = 15 ms
  p90  = 28 ms
  p95  = 35 ms
  p99  = 45 ms
  p99.9 = 67 ms

Throughput: 400 req/s
Success Rate: 99.8%
========================================
```

### Consumer Pipeline

```
[2025-12-11 01:28:00] [INFO] Consumer initialized successfully
[2025-12-11 01:28:05] [INFO] Successfully processed event: evt-demo-001
[2025-12-11 01:28:05] [INFO] ES upsert successful for product: P123
[2025-12-11 01:28:05] [INFO] Redis cache updated for product: P123
```

**Elasticsearch Verification:**
```json
{
  "found": true,
  "_source": {
    "title": "Gaming Laptop Pro",
    "description": "Ultimate gaming laptop with RTX 4090",
    "price": 2499.99,
    "version": 5,
    "updated_at": "2025-12-11T01:00:00Z"
  }
}
```

### Ingestion Demo

**Before Crash:**
```
Wrote batch to: batch_20251211_012800_0.jsonl (100 events)
SinkAPI: Successfully sent batch (100 events)
Deleted batch file: batch_20251211_012800_0.jsonl
Wrote batch to: batch_20251211_012801_1.jsonl (100 events)
[CRASH - Service killed]
```

**After Restart:**
```
AppendLog initialized: ./append-log
Replaying pending batches...
Found 2 pending batches
Replaying batch: batch_20251211_012801_1.jsonl (100 events)
SinkAPI: Successfully sent batch (100 events)
Successfully replayed and deleted: batch_20251211_012801_1.jsonl
Replaying batch: batch_20251211_012802_2.jsonl (100 events)
SinkAPI: Successfully sent batch (100 events)
Successfully replayed and deleted: batch_20251211_012802_2.jsonl
Replay complete
IngestionServer started on port 8081
```

## Cleanup

```bash
# Stop all services
pkill -f search_service
pkill -f consumer_service
pkill -f ingest_demo

# Stop infrastructure
docker-compose down -v

# Clean build artifacts
rm -rf build/

# Clean append-log
rm -rf append-log/
```

## Key Takeaways

1. **Search Service**: Low-latency search with custom reranking (p99 < 50ms)
2. **Consumer Pipeline**: Idempotent event processing with ES + Redis
3. **Ingestion Demo**: Crash recovery with automatic replay of pending batches
4. **Production Quality**: Error handling, logging, metrics, testing

## Troubleshooting

**Elasticsearch not ready:**
```bash
# Check status
curl http://localhost:9200/_cluster/health

# Wait longer
sleep 30
```

**Kafka not ready:**
```bash
# Check broker
docker exec atlassearch-kafka-1 kafka-broker-api-versions --bootstrap-server localhost:9092
```

**Build errors:**
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev \
  libboost-all-dev librdkafka-dev libhiredis-dev libyaml-cpp-dev
```

## Next Steps

- Explore [design.md](design.md) for architecture details
- Run full benchmark suite: `bash bench/run_bench.sh`
- Review algorithm solutions in `algorithms/cpp/`
- Read service-specific READMEs for detailed usage
