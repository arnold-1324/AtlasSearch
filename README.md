# AtlasSearch

**AtlasSearch** is a production-quality C++ repository demonstrating advanced systems engineering, competitive programming fundamentals, and real-world distributed systems design.

## 🏗️ Architecture Overview

```
┌─────────────┐      ┌──────────────┐      ┌─────────────┐
│   Client    │─────▶│    Search    │─────▶│Elasticsearch│
│  (HTTP)     │      │   Service    │      │             │
└─────────────┘      └──────────────┘      └─────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │   Reranking  │
                     │   Algorithm  │
                     └──────────────┘

┌─────────────┐      ┌──────────────┐      ┌─────────────┐
│   Kafka     │─────▶│   Consumer   │─────▶│Elasticsearch│
│   Topic     │      │   Pipeline   │      │             │
└─────────────┘      └──────────────┘      └─────────────┘
                            │
                            ├──────────────▶┌─────────────┐
                            │               │    Redis    │
                            │               └─────────────┘
                            ▼
                     ┌──────────────┐
                     │     DLQ      │
                     └──────────────┘

┌─────────────┐      ┌──────────────┐      ┌─────────────┐
│   HTTP      │─────▶│   Ingestion  │─────▶│ Append Log  │
│   POST      │      │    Demo      │      │   (JSONL)   │
└─────────────┘      └──────────────┘      └─────────────┘
                            │
                            ▼
                     ┌──────────────┐
                     │  Sink API    │
                     └──────────────┘
```

## 📁 Repository Structure

```
AtlasSearch/
├── README.md
├── CMakeLists.txt
├── docker-compose.yml
├── services/
│   ├── search-service/
│   │   ├── src/
│   │   ├── include/
│   │   ├── tests/
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   ├── consumer-service/
│   │   ├── src/
│   │   ├── include/
│   │   ├── tests/
│   │   ├── config.yml
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   └── ingest-demo/
│       ├── src/
│       ├── include/
│       ├── tests/
│       ├── CMakeLists.txt
│       └── README.md
├── algorithms/
│   ├── cpp/
│   └── README.md
├── bench/
│   ├── src/
│   ├── CMakeLists.txt
│   ├── run_bench.sh
│   └── README.md
├── docs/
│   ├── design.md
│   └── demo_script.md
└── .github/
    └── workflows/
        └── ci.yml
```

## 🚀 Quick Start

### Prerequisites

- **C++17** compiler (GCC 7+, Clang 5+)
- **CMake** 3.14+
- **Docker** & **Docker Compose**
- **libcurl**, **boost**, **nlohmann/json**

### Build All Services

```bash
# Clone the repository
git clone https://github.com/yourusername/AtlasSearch.git
cd AtlasSearch

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

### Start Infrastructure

```bash
# Start Elasticsearch, Kafka, Zookeeper, Redis
docker-compose up -d

# Wait for services to be ready (~30 seconds)
sleep 30
```

### Run Services

#### 1. Search Service

```bash
# From build directory
./services/search-service/search_service

# Test endpoint
curl "http://localhost:8080/search?q=laptop&size=10"
```

#### 2. Consumer Pipeline

```bash
# From build directory
./services/consumer-service/consumer_service ../services/consumer-service/config.yml
```

#### 3. Ingestion Demo

```bash
# From build directory
./services/ingest-demo/ingest_demo

# Send test events
./services/ingest-demo/producer_tool --rate=100 --burst=500
```

## 📊 90-Second Demo Script

See [docs/demo_script.md](docs/demo_script.md) for the complete walkthrough.

**Quick Demo:**

```bash
# Terminal 1: Start infrastructure
docker-compose up -d && sleep 30

# Terminal 2: Build and run search service
mkdir build && cd build && cmake .. && make -j4
./services/search-service/search_service

# Terminal 3: Run benchmark
./bench/bench_tool --concurrency=10 --requests=1000 --url=http://localhost:8080/search?q=test

# Expected output:
# p50 = 15 ms
# p90 = 28 ms
# p99 = 45 ms

# Terminal 4: Test ingestion with replay
./services/ingest-demo/ingest_demo &
./services/ingest-demo/producer_tool --rate=50 --burst=200
# Kill the service (Ctrl+C)
# Restart - should replay pending events
./services/ingest-demo/ingest_demo
```

## 🔍 Example Usage

### Search Service

```bash
# Basic search
curl "http://localhost:8080/search?q=laptop&size=10"

# Response:
{
  "results": [
    {
      "id": "P123",
      "title": "Gaming Laptop",
      "description": "High-performance laptop",
      "score": 8.5,
      "es_score": 7.2,
      "recency_score": 0.9,
      "title_match_score": 1.0
    }
  ],
  "latency_ms": 23,
  "total": 150
}
```

### Consumer Pipeline

```bash
# Produce test event to Kafka
docker exec -it atlassearch-kafka-1 kafka-console-producer \
  --broker-list localhost:9092 \
  --topic product-events

# Paste JSON:
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
```

### Benchmark Results (Example)

```
========================================
AtlasSearch Benchmark Results
========================================
URL: http://localhost:8080/search?q=test
Concurrency: 10
Total Requests: 1000
Duration: 2.5s

Latency Distribution:
  p50 = 15 ms
  p90 = 28 ms
  p95 = 35 ms
  p99 = 45 ms
  p99.9 = 67 ms

Throughput: 400 req/s
Success Rate: 99.8%
========================================
```

## 🧪 Running Tests

```bash
# From build directory
cd build

# Run all tests
ctest --output-on-failure

# Run specific service tests
./services/search-service/tests/search_service_tests
./services/consumer-service/tests/consumer_service_tests
./services/ingest-demo/tests/ingest_demo_tests
```

## 🧮 Algorithms

The repository includes 30+ competitive programming solutions in C++:

- **Arrays**: Two Sum, Kadane's Algorithm, Merge Intervals
- **Strings**: Longest Substring Without Repeating, KMP Pattern Matching
- **Trees**: Inorder/Preorder/Postorder, BFS, DFS, Diameter, LCA
- **Graphs**: Dijkstra, BFS Shortest Path, Union Find, Cycle Detection
- **Dynamic Programming**: LIS, Knapsack, Coin Change
- **Advanced**: Trie Autocomplete, Top-K Elements, Sliding Window

See [algorithms/README.md](algorithms/README.md) for the complete list.

## 🐳 Docker Services

| Service | Port | Description |
|---------|------|-------------|
| Elasticsearch | 9200 | Document store and search engine |
| Kafka | 9092 | Event streaming platform |
| Zookeeper | 2181 | Kafka coordination |
| Redis | 6379 | Cache layer |

## 📚 Documentation

- [Design Document](docs/design.md) - Architecture and design decisions
- [Demo Script](docs/demo_script.md) - Step-by-step demonstration
- [Search Service README](services/search-service/README.md)
- [Consumer Service README](services/consumer-service/README.md)
- [Ingestion Demo README](services/ingest-demo/README.md)

## 🎯 Key Features

### Search Service
- ✅ HTTP REST API with cpp-httplib
- ✅ Elasticsearch integration with libcurl
- ✅ Custom reranking algorithm (ES score + recency + title match)
- ✅ Connection pooling and retry logic
- ✅ Comprehensive error handling
- ✅ Unit tests with GoogleTest

### Consumer Pipeline
- ✅ Kafka consumption with librdkafka
- ✅ Idempotent processing with version checking
- ✅ Manual offset commit after success
- ✅ Redis cache invalidation
- ✅ Dead Letter Queue for failed events
- ✅ Exponential backoff retry
- ✅ Structured logging

### Ingestion Demo
- ✅ Lock-free bounded queue
- ✅ HTTP backpressure (429 on full queue)
- ✅ Batching with time/size triggers
- ✅ Durable append-log (JSONL)
- ✅ Crash recovery with replay
- ✅ Configurable batch parameters

## 🏆 Production Quality

This repository demonstrates:

1. **Advanced C++ Systems Engineering**
   - Modern C++17 idioms
   - RAII and smart pointers
   - Lock-free data structures
   - Efficient memory management

2. **Algorithms & Competitive Programming**
   - 30+ solved problems
   - Optimal time/space complexity
   - Clean, well-documented code

3. **Distributed Systems Design**
   - Event-driven architecture
   - Idempotency and exactly-once semantics
   - Backpressure handling
   - Crash recovery
   - Observability (metrics, logging)

## 📝 License

MIT License - See LICENSE file for details

## 🤝 Contributing

Contributions are welcome! Please read CONTRIBUTING.md for guidelines.

## 📧 Contact

For questions or feedback, please open an issue on GitHub.
