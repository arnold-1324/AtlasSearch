# Consumer Service

Production-style Kafka → Elasticsearch → Redis ingestion pipeline with idempotency and manual offset commit.

## Features

- ✅ Kafka consumption using librdkafka (rdkafka++)
- ✅ Idempotent processing with version/timestamp checking
- ✅ Manual offset commit after successful processing
- ✅ Elasticsearch upsert with exponential backoff retry
- ✅ Redis cache management with invalidation on failure
- ✅ Dead Letter Queue (DLQ) for failed events
- ✅ Structured logging and metrics
- ✅ Graceful shutdown

## Architecture

```
┌─────────────┐
│   Kafka     │
│   Topic     │
└──────┬──────┘
       │
       ▼
┌──────────────┐
│   Consumer   │
│   (librdkafka)│
└──────┬───────┘
       │
       ├──────────────────────────────┐
       │                              │
       ▼                              ▼
┌──────────────┐              ┌──────────────┐
│     ES       │              │    Redis     │
│   Upsert     │              │    Cache     │
└──────────────┘              └──────────────┘
       │
       │ (on failure)
       ▼
┌──────────────┐
│     DLQ      │
│    Topic     │
└──────────────┘
```

## Event Format

```json
{
  "product_id": "P123",
  "event_id": "evt-uuid-001",
  "event_type": "create|update|delete",
  "version": 12,
  "updated_at": "2025-12-11T10:23:00Z",
  "data": {
    "title": "Product Title",
    "description": "Product description",
    "price": 99.99,
    "category": "Electronics"
  }
}
```

## Idempotency

The consumer ensures exactly-once semantics by:

1. Fetching existing document from Elasticsearch
2. Comparing `version` and `updated_at` fields
3. Skipping events with older or equal version/timestamp
4. Only committing Kafka offset after successful ES + Redis update

## Retry Logic

Elasticsearch upserts use exponential backoff:
- Attempt 1: immediate
- Attempt 2: 100ms delay
- Attempt 3: 200ms delay
- After max retries: send to DLQ

## Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install librdkafka-dev libhiredis-dev libyaml-cpp-dev

# From repository root
mkdir build && cd build
cmake ..
make consumer_service

# Run
./services/consumer-service/consumer_service ../services/consumer-service/config.yml
```

## Configuration

Edit `config.yml`:

```yaml
kafka:
  brokers: "localhost:9092"
  group_id: "atlas-product-consumer"
  topic: "product-events"
  dlq_topic: "product-dlq"

elasticsearch:
  host: "localhost"
  port: 9200

redis:
  host: "localhost"
  port: 6379
```

## Testing

### Unit Tests

```bash
# From build directory
./services/consumer-service/consumer_service_tests
```

### Integration Test

```bash
# Terminal 1: Start infrastructure
docker-compose up -d

# Terminal 2: Start consumer
./build/services/consumer-service/consumer_service services/consumer-service/config.yml

# Terminal 3: Produce test event
docker exec -it atlassearch-kafka-1 kafka-console-producer \
  --broker-list localhost:9092 \
  --topic product-events

# Paste event:
{
  "product_id": "P123",
  "event_id": "evt-001",
  "event_type": "update",
  "version": 5,
  "updated_at": "2025-12-11T01:00:00Z",
  "data": {
    "title": "Gaming Laptop Pro",
    "description": "Ultimate gaming experience",
    "price": 1299.99
  }
}

# Verify in Elasticsearch
curl http://localhost:9200/products/_doc/P123

# Verify in Redis
docker exec -it atlassearch-redis-1 redis-cli GET product:P123
```

## Monitoring

The consumer logs structured events:

```
[2025-12-11 01:28:00] [INFO] Consumer initialized successfully
[2025-12-11 01:28:05] [INFO] Successfully processed event: evt-001
[2025-12-11 01:28:10] [METRICS] events_processed: 100
```

Metrics counters:
- `events_processed`: Successfully processed events
- `events_failed`: Events sent to DLQ
- `events_parse_error`: Malformed events

## Dead Letter Queue

Failed events are sent to the DLQ topic with metadata:

```json
{
  "original_event": "{...}",
  "error_reason": "ES upsert failed after 3 retries",
  "timestamp": 1702261680
}
```

## Graceful Shutdown

The consumer handles SIGINT and SIGTERM:

```bash
# Send SIGINT (Ctrl+C)
# Consumer will:
# 1. Stop consuming new messages
# 2. Complete processing current message
# 3. Commit final offset
# 4. Close connections
```

## Performance

Expected throughput:
- ~1000-2000 events/sec (single consumer)
- Latency: 10-50ms per event (including ES + Redis)

For higher throughput, run multiple consumer instances with the same `group_id`.
