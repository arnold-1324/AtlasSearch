# Search Service

Production-quality C++ search API service with Elasticsearch integration and custom reranking.

## Features

- ✅ HTTP REST API using cpp-httplib
- ✅ Elasticsearch integration with libcurl
- ✅ Multi-match query on `title^3` + `description`
- ✅ Custom reranking algorithm
- ✅ Connection pooling and retry logic
- ✅ Comprehensive error handling
- ✅ Unit tests with GoogleTest

## Reranking Algorithm

The service applies a custom reranking step after retrieving Elasticsearch results:

```
final_score = 0.7 * es_score + 0.2 * recency_score + 0.1 * title_match_score
```

Where:
- **es_score**: Elasticsearch relevance score from multi_match query
- **recency_score**: Exponential decay based on document age (e^(-days/30))
- **title_match_score**: Exact/partial match ratio of query terms in title

## Building

```bash
# From repository root
mkdir build && cd build
cmake ..
make search_service

# Run
./services/search-service/search_service
```

## API Endpoints

### GET /search

Search for products with custom reranking.

**Parameters:**
- `q` (required): Search query string
- `size` (optional): Number of results (default: 10, max: 100)

**Example:**
```bash
curl "http://localhost:8080/search?q=laptop&size=5"
```

**Response:**
```json
{
  "results": [
    {
      "id": "P123",
      "title": "Gaming Laptop Pro",
      "description": "High-performance gaming laptop",
      "score": 8.5,
      "es_score": 7.2,
      "recency_score": 0.95,
      "title_match_score": 1.0,
      "updated_at": "2025-12-10T10:00:00Z"
    }
  ],
  "total": 42,
  "latency_ms": 23,
  "query": "laptop",
  "size": 5
}
```

### GET /health

Health check endpoint.

**Example:**
```bash
curl http://localhost:8080/health
```

**Response:**
```json
{
  "status": "healthy",
  "service": "atlas-search",
  "version": "1.0.0"
}
```

## Running Tests

```bash
# From build directory
./services/search-service/search_service_tests
```

## Configuration

The service connects to Elasticsearch at `localhost:9200` by default. To change this, modify the initialization in `main.cpp`:

```cpp
atlas::SearchService search_service("your-es-host", 9200);
```

## Dependencies

- C++17 compiler
- libcurl
- cpp-httplib
- nlohmann/json
- GoogleTest (for tests)

## Performance

Expected latency with local Elasticsearch:
- p50: ~15ms
- p90: ~28ms
- p99: ~45ms

See [bench/](../../bench/) for benchmarking tools.
