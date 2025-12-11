# Benchmark Tool

Multi-threaded HTTP benchmark tool for measuring search service performance.

## Features

- ✅ Concurrent HTTP requests
- ✅ Latency percentile calculation (p50, p90, p95, p99, p99.9)
- ✅ Throughput measurement
- ✅ CSV output for analysis
- ✅ Configurable concurrency and request count

## Building

```bash
# From repository root
mkdir build && cd build
cmake ..
make bench_tool
```

## Usage

```bash
./bench/bench_tool [options]

Options:
  --url <url>           Target URL (default: http://localhost:8080/search?q=test&size=10)
  --concurrency <n>     Number of concurrent threads (default: 10)
  --requests <n>        Total number of requests (default: 1000)
  --help                Show help message
```

## Examples

### Basic Benchmark

```bash
./build/bench/bench_tool \
  --url="http://localhost:8080/search?q=laptop&size=10" \
  --concurrency=10 \
  --requests=1000
```

### High Concurrency Test

```bash
./build/bench/bench_tool \
  --url="http://localhost:8080/search?q=test" \
  --concurrency=100 \
  --requests=10000
```

### Low Concurrency (Baseline)

```bash
./build/bench/bench_tool \
  --concurrency=1 \
  --requests=100
```

## Sample Output

```
========================================
AtlasSearch Benchmark Tool
========================================
URL: http://localhost:8080/search?q=laptop&size=10
Concurrency: 10
Total Requests: 1000
========================================

========================================
Benchmark Results
========================================
Duration: 2.50s

Requests:
  Total: 1000
  Success: 998
  Errors: 2
  Success Rate: 99.8%

Latency Distribution:
  p50  = 15 ms
  p90  = 28 ms
  p95  = 35 ms
  p99  = 45 ms
  p99.9 = 67 ms

Throughput: 400 req/s
========================================

CSV Format:
url,concurrency,requests,duration_sec,success,errors,p50,p90,p95,p99,throughput
http://localhost:8080/search?q=laptop&size=10,10,1000,2.50,998,2,15,28,35,45,400
```

## Interpreting Results

### Latency Percentiles

- **p50 (median)**: Half of requests complete faster than this
- **p90**: 90% of requests complete faster than this
- **p95**: 95% of requests complete faster than this
- **p99**: 99% of requests complete faster than this (important for SLA)
- **p99.9**: 99.9% of requests complete faster than this

### Throughput

Requests per second the service can handle at the given concurrency level.

### Success Rate

Percentage of requests that completed successfully (HTTP 200).

## Benchmark Suite

Run the complete benchmark suite:

```bash
bash bench/run_bench.sh
```

This runs multiple configurations:
- Light load: 10 concurrent, 1000 requests
- Medium load: 50 concurrent, 5000 requests
- Heavy load: 100 concurrent, 10000 requests

## Tips

1. **Warm up**: Run a small benchmark first to warm up caches
2. **Consistent load**: Keep Elasticsearch/Redis load consistent
3. **Network**: Run on same machine or low-latency network
4. **Multiple runs**: Average results from multiple runs
5. **Monitor**: Watch CPU/memory during benchmarks

## CSV Analysis

Export results to CSV for analysis:

```bash
./build/bench/bench_tool --concurrency=10 --requests=1000 | grep "^http" > results.csv
```

Then analyze in Excel, Python, or R.
