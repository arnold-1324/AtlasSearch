# Ingestion Demo

Durable ingestion server demonstrating crash recovery with append-log and backpressure handling.

## Features

- ✅ HTTP POST /events endpoint
- ✅ Lock-free bounded queue (boost::lockfree::queue)
- ✅ HTTP 429 backpressure when queue is full
- ✅ Background batcher (size + time triggers)
- ✅ Durable JSONL append-log
- ✅ Crash recovery with automatic replay
- ✅ Simulated sink API
- ✅ Producer tool for load testing

## Architecture

```
┌─────────────┐
│   HTTP      │
│   POST      │
└──────┬──────┘
       │
       ▼
┌──────────────┐
│  Lock-Free   │  ──429──▶ Backpressure
│    Queue     │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│   Batcher    │
│ (100 events  │
│  or 1 sec)   │
└──────┬───────┘
       │
       ├──────────────────┐
       │                  │
       ▼                  ▼
┌──────────────┐   ┌──────────────┐
│ Append Log   │   │  Sink API    │
│  (JSONL)     │   │              │
└──────────────┘   └──────┬───────┘
       │                  │
       │    Success       │
       └◀─────────────────┘
       │
       ▼
   Delete Log
```

## Building

```bash
# From repository root
mkdir build && cd build
cmake ..
make ingest_demo producer_tool

# Run
./services/ingest-demo/ingest_demo
```

## Usage

### Start Server

```bash
./build/services/ingest-demo/ingest_demo
```

Configuration (hardcoded in main.cpp):
- Port: 8081
- Queue size: 10,000 events
- Batch size: 100 events
- Batch timeout: 1 second
- Log directory: `./append-log`

### Send Events

```bash
curl -X POST http://localhost:8081/events \
  -H "Content-Type: application/json" \
  -d '{
    "id": "evt-001",
    "type": "user_action",
    "data": {
      "user_id": "U123",
      "action": "click",
      "timestamp": 1702261680
    }
  }'
```

**Response (202 Accepted):**
```json
{"status": "accepted"}
```

**Response (429 Too Many Requests):**
```json
{"error": "Queue full, please retry later"}
```

### Load Testing with Producer Tool

```bash
# Send 500 events at 100 events/sec
./build/services/ingest-demo/producer_tool --rate=100 --burst=500

# High load test
./build/services/ingest-demo/producer_tool --rate=1000 --burst=10000 --workers=4

# Custom URL
./build/services/ingest-demo/producer_tool --url=http://localhost:8081/events --rate=200 --burst=1000
```

**Producer Options:**
- `--url <url>`: Target URL (default: http://localhost:8081/events)
- `--rate <n>`: Events per second per worker (default: 100)
- `--burst <n>`: Total events to send (default: 500)
- `--workers <n>`: Number of worker threads (default: 1)

## 90-Second Demo: Crash Recovery

This demo shows the durable append-log and replay mechanism.

### Step 1: Start Server

```bash
./build/services/ingest-demo/ingest_demo
```

Output:
```
AppendLog initialized: ./append-log
Replaying pending batches...
No pending batches to replay
Replay complete
IngestionServer started on port 8081
```

### Step 2: Send Events

```bash
# In another terminal
./build/services/ingest-demo/producer_tool --rate=50 --burst=200
```

You'll see batches being written and sent:
```
Wrote batch to: batch_20251211_012800_0.jsonl (100 events)
SinkAPI: Successfully sent batch (100 events)
Deleted batch file: batch_20251211_012800_0.jsonl
```

### Step 3: Simulate Failure

While events are being sent, **kill the server** (Ctrl+C or kill -9):

```bash
# The server crashes mid-batch
# Some batch files remain in ./append-log/
```

### Step 4: Restart Server

```bash
./build/services/ingest-demo/ingest_demo
```

**Expected Output:**
```
AppendLog initialized: ./append-log
Replaying pending batches...
Found 2 pending batches
Replaying batch: batch_20251211_012800_1.jsonl (100 events)
SinkAPI: Successfully sent batch (100 events)
Successfully replayed and deleted: batch_20251211_012800_1.jsonl
Replaying batch: batch_20251211_012800_2.jsonl (37 events)
SinkAPI: Successfully sent batch (37 events)
Successfully replayed and deleted: batch_20251211_012800_2.jsonl
Replay complete
IngestionServer started on port 8081
```

✅ **Result**: All pending events are replayed before accepting new traffic!

## Backpressure Demo

### Step 1: Start Server

```bash
./build/services/ingest-demo/ingest_demo
```

### Step 2: Flood with High Rate

```bash
./build/services/ingest-demo/producer_tool --rate=5000 --burst=50000 --workers=10
```

**Expected Output:**
```
========================================
Producer Statistics
========================================
Total sent: 50000
Accepted (202): 45230
Backpressure (429): 4770
Errors: 0
Duration: 12500 ms
Throughput: 4000 events/sec
========================================
```

✅ **Result**: Server returns HTTP 429 when queue is full, implementing proper backpressure!

## Append Log Format

Batches are stored as JSONL (JSON Lines) files:

**File**: `append-log/batch_20251211_012800_0.jsonl`

```json
{"id":"evt-0-0","type":"test","data":{"message":"Test event","worker_id":0,"sequence":0},"timestamp":1702261680000}
{"id":"evt-0-1","type":"test","data":{"message":"Test event","worker_id":0,"sequence":1},"timestamp":1702261680100}
{"id":"evt-0-2","type":"test","data":{"message":"Test event","worker_id":0,"sequence":2},"timestamp":1702261680200}
```

## Testing

```bash
# From build directory
./services/ingest-demo/ingest_demo_tests
```

Tests cover:
- Append-log write/read/delete
- Batch flushing (size and time triggers)
- Sink API simulation
- Replay mechanism

## Configuration

To modify configuration, edit `src/main.cpp`:

```cpp
int port = 8081;
int queue_size = 10000;      // Bounded queue capacity
int batch_size = 100;         // Flush after N events
int batch_wait_ms = 1000;     // Flush after N milliseconds
std::string log_dir = "./append-log";
```

## Performance

Expected performance on modern hardware:
- Ingestion rate: 5,000-10,000 events/sec
- Batch latency: <1ms (queue to batcher)
- Replay rate: 10,000+ events/sec

## Production Considerations

This is a demo implementation. For production:

1. **Persistent Queue**: Use RocksDB or similar for queue persistence
2. **Distributed Coordination**: Use Zookeeper for multi-instance coordination
3. **Monitoring**: Add Prometheus metrics
4. **Compression**: Compress JSONL files
5. **Retention**: Implement log rotation and cleanup
6. **Sink Retry**: More sophisticated retry with circuit breaker
7. **Checkpointing**: Periodic checkpoints for faster recovery
