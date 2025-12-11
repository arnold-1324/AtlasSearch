# AtlasSearch - Complete Repository Structure

```
AtlasSearch/
│
├── README.md                          # Main project documentation
├── CMakeLists.txt                     # Root CMake configuration
├── docker-compose.yml                 # Infrastructure services
├── .gitignore                         # Git ignore patterns
│
├── .github/
│   └── workflows/
│       └── ci.yml                     # GitHub Actions CI/CD pipeline
│
├── services/
│   ├── search-service/
│   │   ├── CMakeLists.txt            # Search service build config
│   │   ├── README.md                 # Search service documentation
│   │   ├── include/
│   │   │   └── search_service.h      # Search service header
│   │   ├── src/
│   │   │   ├── main.cpp              # HTTP server entry point
│   │   │   └── search_service.cpp    # Search + reranking implementation
│   │   └── tests/
│   │       └── search_service_test.cpp  # Unit tests
│   │
│   ├── consumer-service/
│   │   ├── CMakeLists.txt            # Consumer service build config
│   │   ├── README.md                 # Consumer service documentation
│   │   ├── config.yml                # Kafka/ES/Redis configuration
│   │   ├── include/
│   │   │   └── consumer.h            # Consumer pipeline header
│   │   ├── src/
│   │   │   ├── main.cpp              # Consumer entry point
│   │   │   └── consumer.cpp          # Kafka→ES→Redis pipeline
│   │   └── tests/
│   │       └── consumer_test.cpp     # Unit tests
│   │
│   └── ingest-demo/
│       ├── CMakeLists.txt            # Ingestion demo build config
│       ├── README.md                 # Ingestion demo documentation
│       ├── include/
│       │   └── ingest_server.h       # Ingestion server header
│       ├── src/
│       │   ├── main.cpp              # Ingestion server entry point
│       │   ├── ingest_server.cpp     # Append-log + replay logic
│       │   └── producer_tool.cpp     # Load testing tool
│       └── tests/
│           └── ingest_test.cpp       # Unit tests
│
├── algorithms/
│   ├── README.md                     # Algorithms overview + list
│   └── cpp/
│       ├── two_sum.cpp               # Hash map approach
│       ├── kadane_max_subarray.cpp   # Maximum subarray sum
│       ├── merge_intervals.cpp       # Interval merging
│       ├── longest_substring_no_repeat.cpp  # Sliding window
│       ├── knapsack_01.cpp           # 0/1 Knapsack DP
│       ├── coin_change.cpp           # Minimum coins DP
│       ├── longest_increasing_subsequence.cpp  # LIS with binary search
│       ├── tree_traversals.cpp       # Inorder/preorder/postorder
│       ├── tree_bfs.cpp              # Level order traversal
│       ├── lowest_common_ancestor.cpp  # LCA in binary tree
│       ├── dijkstra_shortest_path.cpp  # Dijkstra's algorithm
│       ├── union_find.cpp            # Disjoint set union
│       ├── detect_cycle.cpp          # Cycle detection in graph
│       ├── trie_autocomplete.cpp     # Trie with autocomplete
│       └── top_k_elements.cpp        # Top K with heap
│       # ... (30+ total algorithm solutions)
│
├── bench/
│   ├── CMakeLists.txt                # Benchmark tool build config
│   ├── README.md                     # Benchmark documentation
│   ├── run_bench.sh                  # Benchmark suite script
│   └── src/
│       └── bench_tool.cpp            # Multi-threaded HTTP benchmark
│
└── docs/
    ├── design.md                     # Architecture & design decisions
    └── demo_script.md                # 90-second demo walkthrough
```

## File Count Summary

### Source Files
- **Headers (.h)**: 3
- **Implementation (.cpp)**: 30+
- **CMake files**: 5
- **YAML configs**: 2

### Documentation
- **README files**: 7
- **Design docs**: 2
- **Total markdown**: 9

### Configuration
- **Docker Compose**: 1
- **CI/CD workflows**: 1
- **Git config**: 1

### Tests
- **Unit test files**: 3
- **Test framework**: GoogleTest

## Lines of Code (Estimated)

| Component | Files | LOC |
|-----------|-------|-----|
| Search Service | 3 | ~600 |
| Consumer Service | 3 | ~800 |
| Ingestion Demo | 4 | ~700 |
| Algorithms | 15+ | ~1500 |
| Benchmark Tool | 1 | ~300 |
| Tests | 3 | ~400 |
| **Total** | **30+** | **~4300** |

## Key Technologies

### Languages
- C++17 (primary)
- Bash (scripts)
- YAML (configuration)
- Markdown (documentation)

### Build System
- CMake 3.14+
- Make

### Libraries
- **HTTP**: cpp-httplib
- **JSON**: nlohmann/json
- **Kafka**: librdkafka (rdkafka++)
- **Redis**: hiredis
- **Curl**: libcurl
- **Concurrency**: boost::lockfree
- **Testing**: GoogleTest
- **Config**: yaml-cpp

### Infrastructure
- Elasticsearch 8.x
- Apache Kafka 7.5
- Redis 7.2
- Zookeeper (for Kafka)

### CI/CD
- GitHub Actions
- Docker Compose

## Build Artifacts

When built, the following executables are generated:

```
build/
├── services/
│   ├── search-service/
│   │   ├── search_service              # Search API server
│   │   └── search_service_tests        # Unit tests
│   ├── consumer-service/
│   │   ├── consumer_service            # Kafka consumer
│   │   └── consumer_service_tests      # Unit tests
│   └── ingest-demo/
│       ├── ingest_demo                 # Ingestion server
│       ├── producer_tool               # Load generator
│       └── ingest_demo_tests           # Unit tests
└── bench/
    └── bench_tool                      # HTTP benchmark tool
```

## Runtime Artifacts

During execution, the following directories/files are created:

```
append-log/                    # Durable event log (JSONL files)
├── batch_20251211_012800_0.jsonl
├── batch_20251211_012801_1.jsonl
└── ...
```

## Documentation Coverage

Every major component has:
1. ✅ README with usage instructions
2. ✅ Code comments explaining logic
3. ✅ Example usage in main()
4. ✅ Complexity analysis
5. ✅ Build instructions

## Testing Coverage

- ✅ Unit tests for all services
- ✅ Integration tests in CI/CD
- ✅ Smoke tests for endpoints
- ✅ Benchmark tests for performance

## Production Readiness Checklist

- ✅ Error handling (timeouts, retries)
- ✅ Logging (structured, leveled)
- ✅ Metrics (counters, latency)
- ✅ Testing (unit, integration)
- ✅ Documentation (README, design docs)
- ✅ CI/CD (automated build + test)
- ✅ Containerization (Docker Compose)
- ⚠️ Authentication (not implemented - demo)
- ⚠️ TLS/SSL (not implemented - demo)
- ⚠️ Rate limiting (not implemented - demo)

## Quick Start Commands

```bash
# 1. Start infrastructure
docker-compose up -d

# 2. Build all services
mkdir build && cd build
cmake .. && make -j$(nproc)

# 3. Run search service
./services/search-service/search_service

# 4. Run benchmark
./bench/bench_tool --concurrency=10 --requests=1000

# 5. Run tests
ctest --output-on-failure
```

## Repository Statistics

- **Total files**: 50+
- **Total lines**: ~5000+ (code + docs)
- **Languages**: C++, Bash, YAML, Markdown
- **Services**: 3 production-style services
- **Algorithms**: 15+ competitive programming solutions
- **Tests**: 3 test suites with GoogleTest
- **Documentation**: 9 markdown files

---

This repository demonstrates **production-quality C++ engineering** with:
- Advanced systems programming
- Distributed systems patterns
- Algorithm expertise
- Testing and CI/CD
- Comprehensive documentation
