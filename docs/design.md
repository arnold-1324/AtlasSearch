# AtlasSearch Design Document

## Overview

AtlasSearch is a production-quality C++ repository demonstrating three core competencies:
1. Advanced C++ systems engineering
2. Algorithms and competitive programming fundamentals
3. Real-world distributed systems design with observability

## System Architecture

### High-Level Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Client Layer                             │
│  (HTTP Clients, Benchmark Tools, Producer Tools)            │
└────────────┬────────────────────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────────────────┐
│                   Service Layer                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Search     │  │   Consumer   │  │  Ingestion   │      │
│  │   Service    │  │   Pipeline   │  │    Demo      │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└────────────┬────────────────┬────────────────┬──────────────┘
             │                │                │
             ▼                ▼                ▼
┌─────────────────────────────────────────────────────────────┐
│                  Infrastructure Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │Elasticsearch │  │    Kafka     │  │    Redis     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

## Component Design

### 1. Search Service

**Purpose**: Provide low-latency search API with custom reranking

**Key Design Decisions**:

1. **HTTP Server**: cpp-httplib chosen for simplicity and header-only deployment
2. **Elasticsearch Client**: Custom implementation using libcurl for full control
3. **Reranking Algorithm**: 
   - Formula: `score = 0.7 * es_score + 0.2 * recency + 0.1 * title_match`
   - Rationale: Balance relevance (ES score) with freshness and exact matching
   - Recency uses exponential decay: `e^(-days/30)`

4. **Connection Pooling**: Implicit via libcurl's connection reuse
5. **Error Handling**: Try-catch with graceful degradation

**Performance Characteristics**:
- Target latency: p99 < 50ms
- Throughput: 500+ req/s on single instance
- Memory: ~50MB baseline

### 2. Consumer Pipeline

**Purpose**: Reliable event processing from Kafka to Elasticsearch + Redis

**Key Design Decisions**:

1. **Idempotency**: 
   - Compare `version` field (monotonically increasing)
   - Compare `updated_at` timestamp
   - Skip events with older/equal values
   - Prevents duplicate processing on retries

2. **Manual Offset Commit**:
   - Only commit after ES + Redis success
   - Ensures at-least-once delivery
   - Combined with idempotency → exactly-once semantics

3. **Retry Strategy**:
   - Exponential backoff: 100ms, 200ms, 400ms
   - Max 3 retries
   - After max retries → send to DLQ

4. **Cache Invalidation**:
   - On ES write failure → delete Redis key
   - Forces cache miss and reload from ES
   - Prevents stale cache

5. **Dead Letter Queue**:
   - Separate Kafka topic for failed events
   - Includes original event + error metadata
   - Allows manual inspection and replay

**Failure Scenarios**:

| Scenario | Behavior |
|----------|----------|
| ES unavailable | Retry with backoff, then DLQ |
| Redis unavailable | Log error, continue (cache miss acceptable) |
| Kafka rebalance | Graceful shutdown, rejoin |
| Duplicate event | Skip via idempotency check |
| Malformed JSON | Parse error → DLQ |

### 3. Ingestion Demo

**Purpose**: Demonstrate durable ingestion with crash recovery

**Key Design Decisions**:

1. **Lock-Free Queue**:
   - `boost::lockfree::queue` for high throughput
   - Bounded capacity for backpressure
   - Wait-free for producers

2. **Backpressure**:
   - HTTP 429 when queue is full
   - Clients implement retry with exponential backoff
   - Prevents OOM

3. **Batching**:
   - Flush on size (100 events) OR time (1 second)
   - Reduces I/O overhead
   - Balances latency vs throughput

4. **Append-Log**:
   - JSONL format (one JSON per line)
   - Atomic file writes
   - Filename includes timestamp for ordering

5. **Crash Recovery**:
   - On startup: scan log directory
   - Replay all pending files in order
   - Delete file only after sink success
   - Ensures no data loss

**State Machine**:

```
Event Received → Queue → Batcher → Append Log → Sink API
                  ↓                    ↓           ↓
                 429              File Created   Success
                                                   ↓
                                              Delete File
```

## Technology Choices

### C++17

**Why C++17?**
- Modern features: structured bindings, `std::optional`, `std::filesystem`
- Mature ecosystem with excellent performance
- Industry standard for systems programming
- Demonstrates low-level systems expertise

### External Dependencies

| Library | Purpose | Rationale |
|---------|---------|-----------|
| cpp-httplib | HTTP server | Header-only, simple API |
| libcurl | HTTP client | Industry standard, battle-tested |
| librdkafka | Kafka client | Official C/C++ client |
| hiredis | Redis client | Lightweight, fast |
| nlohmann/json | JSON parsing | Modern C++ API, header-only option |
| boost::lockfree | Lock-free queue | High-performance concurrency |
| GoogleTest | Unit testing | Industry standard |

### Infrastructure

**Elasticsearch**: Document store with full-text search capabilities
**Kafka**: Event streaming for reliable message delivery
**Redis**: In-memory cache for low-latency reads
**Docker Compose**: Local development environment

## Distributed Systems Patterns

### 1. Idempotency

**Problem**: Network failures cause retries, leading to duplicate processing

**Solution**: Version/timestamp checking
- Each event has monotonically increasing version
- Consumer compares with existing document
- Skips if version ≤ existing

**Benefits**:
- Safe retries
- Exactly-once semantics
- No duplicate side effects

### 2. Dead Letter Queue

**Problem**: Some events fail permanently (malformed, business logic errors)

**Solution**: Separate topic for failed events
- Preserves original event
- Adds error metadata
- Allows manual inspection

**Benefits**:
- Don't block pipeline on bad events
- Audit trail for failures
- Can replay after fixing issue

### 3. Backpressure

**Problem**: Fast producers overwhelm slow consumers

**Solution**: Bounded queue with HTTP 429
- Queue has fixed capacity
- Return 429 when full
- Client retries with backoff

**Benefits**:
- Prevents OOM
- Graceful degradation
- Self-regulating system

### 4. Crash Recovery

**Problem**: Process crashes lose in-memory data

**Solution**: Durable append-log
- Write to disk before processing
- Replay on startup
- Delete only after success

**Benefits**:
- No data loss
- Automatic recovery
- Simple implementation

## Performance Considerations

### Search Service

**Optimizations**:
1. Connection reuse (libcurl)
2. Minimal JSON parsing (only required fields)
3. In-memory reranking (no additional I/O)
4. Async-ready design (can add thread pool)

**Bottlenecks**:
- Elasticsearch query time (dominant factor)
- Network latency
- JSON serialization

### Consumer Pipeline

**Optimizations**:
1. Manual offset commit (batch commits possible)
2. Async ES writes (can parallelize)
3. Redis pipelining (can batch operations)

**Bottlenecks**:
- ES write throughput
- Kafka fetch latency
- Network round-trips

### Ingestion Demo

**Optimizations**:
1. Lock-free queue (no mutex contention)
2. Batching (reduces I/O)
3. Async file writes (can use O_DIRECT)

**Bottlenecks**:
- Disk I/O (append-log writes)
- Sink API latency
- Queue contention at high load

## Observability

### Logging

**Structured Logging Format**:
```
[timestamp] [level] message
[2025-12-11 01:28:00] [INFO] Successfully processed event: evt-001
```

**Log Levels**:
- INFO: Normal operations
- ERROR: Failures requiring attention
- METRICS: Performance counters

### Metrics

**Key Metrics**:
- `events_processed`: Total successful events
- `events_failed`: Events sent to DLQ
- `events_parse_error`: Malformed events
- `latency_ms`: Request latency (search service)

**Future Enhancements**:
- Prometheus integration
- Grafana dashboards
- Distributed tracing (OpenTelemetry)

## Testing Strategy

### Unit Tests

**Coverage**:
- Search service: Reranking algorithm, ES client
- Consumer: Idempotency, retry logic, DLQ
- Ingestion: Append-log, batching, replay

**Approach**:
- GoogleTest framework
- Mock external dependencies
- Test edge cases

### Integration Tests

**Scenarios**:
- End-to-end search flow
- Kafka → ES → Redis pipeline
- Crash recovery with replay

**Approach**:
- Docker Compose for infrastructure
- Real services (not mocks)
- Automated in CI/CD

### Performance Tests

**Benchmarks**:
- Search service: Latency percentiles
- Consumer: Throughput (events/sec)
- Ingestion: Queue saturation

**Tools**:
- Custom bench_tool
- Producer tool for load generation

## Security Considerations

**Current State** (Demo):
- No authentication
- No TLS
- No input validation

**Production Requirements**:
1. **Authentication**: OAuth 2.0, API keys
2. **Authorization**: RBAC for endpoints
3. **Encryption**: TLS for all network traffic
4. **Input Validation**: Sanitize all user input
5. **Rate Limiting**: Per-client quotas
6. **Audit Logging**: Track all access

## Scalability

### Horizontal Scaling

**Search Service**:
- Stateless → add more instances
- Load balancer (nginx, HAProxy)
- Shared Elasticsearch cluster

**Consumer Pipeline**:
- Kafka consumer groups
- Partition-based parallelism
- Each consumer processes subset of partitions

**Ingestion Demo**:
- Multiple instances with shared sink
- Distributed coordination (Zookeeper)
- Partitioned append-log

### Vertical Scaling

**Limits**:
- Search: CPU-bound (JSON parsing, reranking)
- Consumer: Network-bound (ES writes)
- Ingestion: Disk I/O-bound (append-log)

## Future Enhancements

1. **Caching Layer**: Redis for search results
2. **ML Reranking**: Neural network for personalization
3. **Streaming Aggregations**: Real-time analytics
4. **Multi-Region**: Geographic distribution
5. **Schema Evolution**: Backward-compatible changes
6. **Circuit Breaker**: Fail fast on downstream errors
7. **Rate Limiting**: Token bucket algorithm
8. **Compression**: Snappy for Kafka messages

## Conclusion

AtlasSearch demonstrates production-quality C++ engineering through:

1. **Clean Architecture**: Separation of concerns, modularity
2. **Reliability**: Idempotency, retries, crash recovery
3. **Performance**: Low latency, high throughput
4. **Observability**: Logging, metrics, testing
5. **Best Practices**: Modern C++, industry-standard tools

This design balances simplicity (for demonstration) with realism (production patterns), making it an excellent showcase of distributed systems expertise.
