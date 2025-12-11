#!/bin/bash

# AtlasSearch Benchmark Script

echo "========================================="
echo "AtlasSearch Benchmark Suite"
echo "========================================="
echo ""

# Check if search service is running
if ! curl -s http://localhost:8080/health > /dev/null 2>&1; then
    echo "Error: Search service is not running on port 8080"
    echo "Please start the service first:"
    echo "  ./build/services/search-service/search_service"
    exit 1
fi

echo "✓ Search service is running"
echo ""

# Benchmark configurations
declare -a CONFIGS=(
    "10:1000:Light Load"
    "50:5000:Medium Load"
    "100:10000:Heavy Load"
)

for config in "${CONFIGS[@]}"; do
    IFS=':' read -r concurrency requests label <<< "$config"
    
    echo "========================================="
    echo "Running: $label"
    echo "Concurrency: $concurrency"
    echo "Requests: $requests"
    echo "========================================="
    
    ./build/bench/bench_tool \
        --url="http://localhost:8080/search?q=laptop&size=10" \
        --concurrency=$concurrency \
        --requests=$requests
    
    echo ""
    sleep 2
done

echo "========================================="
echo "Benchmark suite complete!"
echo "========================================="
