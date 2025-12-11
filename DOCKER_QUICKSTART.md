# AtlasSearch - Docker Quick Start Guide

## ✅ All Issues Resolved!

The repository is now fully configured to run in Docker containers. No need to install CMake or C++ compilers on Windows!

## 🚀 Quick Start (3 Commands)

### Step 1: Build All Services
```powershell
docker-compose build
```
This will:
- Build the Search Service
- Build the Consumer Service  
- Build the Ingestion Demo
- Download all dependencies automatically

**⏱️ Expected time:** 5-10 minutes (first time only)

### Step 2: Start Everything
```powershell
docker-compose up -d
```
This starts:
- ✅ Elasticsearch (port 9200)
- ✅ Kafka + Zookeeper (port 9092)
- ✅ Redis (port 6379)
- ✅ Search Service (port 8080)
- ✅ Consumer Service
- ✅ Ingestion Demo (port 8081)
- ✅ Kafka UI (port 8090)

### Step 3: Test the Search Service
```powershell
curl http://localhost:8080/health
```

Expected response:
```json
{
  "status": "healthy",
  "service": "atlas-search",
  "version": "1.0.0"
}
```

## 📊 Service Endpoints

| Service | Port | Endpoint | Description |
|---------|------|----------|-------------|
| **Search Service** | 8080 | `GET /search?q=term&size=10` | Search API with reranking |
| **Search Service** | 8080 | `GET /health` | Health check |
| **Ingestion Demo** | 8081 | `POST /events` | Event ingestion |
| **Ingestion Demo** | 8081 | `GET /health` | Health check |
| **Elasticsearch** | 9200 | `GET /_cluster/health` | ES cluster health |
| **Kafka UI** | 8090 | Web interface | Kafka monitoring |
| **Redis** | 6379 | Redis protocol | Cache |

## 🧪 Testing the Services

### 1. Test Search Service

First, create a test index and add some data:

```powershell
# Create products index
curl -X PUT "http://localhost:9200/products" -H "Content-Type: application/json" -d '{\"mappings\":{\"properties\":{\"title\":{\"type\":\"text\"},\"description\":{\"type\":\"text\"},\"price\":{\"type\":\"float\"},\"updated_at\":{\"type\":\"date\"}}}}'

# Add test product
curl -X POST "http://localhost:9200/products/_doc/P001" -H "Content-Type: application/json" -d '{\"title\":\"Gaming Laptop\",\"description\":\"High-performance laptop for gaming\",\"price\":1999.99,\"updated_at\":\"2025-12-11T00:00:00Z\"}'

# Refresh index
curl -X POST "http://localhost:9200/products/_refresh"

# Search
curl "http://localhost:8080/search?q=laptop&size=5"
```

### 2. Test Ingestion Demo

```powershell
curl -X POST http://localhost:8081/events -H "Content-Type: application/json" -d '{\"id\":\"evt-001\",\"type\":\"test\",\"data\":{\"message\":\"Hello AtlasSearch!\"}}'
```

Expected response:
```json
{"status": "accepted"}
```

### 3. Test Consumer Pipeline

```powershell
# Produce event to Kafka (requires docker exec)
docker exec -i atlassearch-kafka kafka-console-producer --broker-list localhost:9092 --topic product-events
```

Then paste this JSON and press Enter:
```json
{
  "product_id": "P123",
  "event_id": "evt-001",
  "event_type": "update",
  "version": 5,
  "updated_at": "2025-12-11T01:00:00Z",
  "data": {
    "title": "Gaming Laptop Pro",
    "description": "Ultimate gaming experience",
    "price": 2499.99
  }
}
```

Press Ctrl+C to exit the producer.

Verify in Elasticsearch:
```powershell
curl http://localhost:9200/products/_doc/P123
```

## 📋 Useful Commands

### View Logs
```powershell
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f search-service
docker-compose logs -f consumer-service
docker-compose logs -f ingest-demo
```

### Stop Services
```powershell
docker-compose down
```

### Rebuild After Code Changes
```powershell
# Rebuild specific service
docker-compose build search-service

# Rebuild all
docker-compose build

# Rebuild and restart
docker-compose up -d --build
```

### Check Service Status
```powershell
docker-compose ps
```

### Access Container Shell
```powershell
docker exec -it atlassearch-search-service /bin/bash
```

## 🎯 90-Second Demo

```powershell
# 1. Build (first time only - takes 5-10 min)
docker-compose build

# 2. Start all services
docker-compose up -d

# 3. Wait for services to be ready (30 seconds)
Start-Sleep -Seconds 30

# 4. Create test index
curl -X PUT "http://localhost:9200/products" -H "Content-Type: application/json" -d '{\"mappings\":{\"properties\":{\"title\":{\"type\":\"text\"},\"description\":{\"type\":\"text\"}}}}'

# 5. Add test data
curl -X POST "http://localhost:9200/products/_doc/P001" -H "Content-Type: application/json" -d '{\"title\":\"Laptop\",\"description\":\"Great laptop\",\"updated_at\":\"2025-12-11T00:00:00Z\"}'

# 6. Refresh index
curl -X POST "http://localhost:9200/products/_refresh"

# 7. Search!
curl "http://localhost:8080/search?q=laptop&size=5"

# 8. Test ingestion
curl -X POST http://localhost:8081/events -H "Content-Type: application/json" -d '{\"id\":\"evt-001\",\"type\":\"test\",\"data\":{\"message\":\"Success!\"}}'
```

## 🐛 Troubleshooting

### Build Fails
```powershell
# Clean and rebuild
docker-compose down
docker-compose build --no-cache
```

### Service Won't Start
```powershell
# Check logs
docker-compose logs search-service

# Check if port is in use
netstat -ano | findstr :8080
```

### Elasticsearch Not Ready
```powershell
# Check health
curl http://localhost:9200/_cluster/health

# Wait longer (ES takes ~30 seconds to start)
Start-Sleep -Seconds 30
```

### Port Already in Use
```powershell
# Find process using port
netstat -ano | findstr :8080

# Kill process (replace PID)
taskkill /PID <PID> /F
```

## 📊 What's Running?

After `docker-compose up -d`, you should see:

```
✔ Container atlassearch-elasticsearch      Running
✔ Container atlassearch-zookeeper          Running
✔ Container atlassearch-kafka              Running
✔ Container atlassearch-redis              Running
✔ Container atlassearch-kafka-ui           Running
✔ Container atlassearch-search-service     Running
✔ Container atlassearch-consumer-service   Running
✔ Container atlassearch-ingest-demo        Running
```

## 🎉 Success Indicators

✅ **Search Service**: `curl http://localhost:8080/health` returns `{"status":"healthy"}`  
✅ **Ingestion Demo**: `curl http://localhost:8081/health` returns `{"status":"healthy"}`  
✅ **Elasticsearch**: `curl http://localhost:9200` returns cluster info  
✅ **Kafka UI**: Open http://localhost:8090 in browser  

## 📚 Next Steps

1. **Explore the code** - All source code is in `services/`
2. **Read the docs** - Check `docs/design.md` for architecture
3. **Run benchmarks** - Build the bench tool (optional)
4. **Review algorithms** - Check `algorithms/cpp/` for solutions
5. **Customize** - Modify services and rebuild

## 💡 Pro Tips

- **First build is slow** - Docker downloads and compiles everything (5-10 min)
- **Subsequent builds are fast** - Docker caches layers
- **Use `--build` flag** - To rebuild after code changes
- **Check logs often** - `docker-compose logs -f` is your friend
- **Kafka UI is helpful** - Browse to http://localhost:8090 to see Kafka topics

---

**You're all set!** 🚀

Run `docker-compose build` to get started!
