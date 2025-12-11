#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include <librdkafka/rdkafkacpp.h>

namespace atlas {

struct ProductEvent {
    std::string product_id;
    std::string event_id;
    std::string event_type; // create, update, delete
    int version;
    std::string updated_at;
    nlohmann::json data;
};

class ElasticsearchWriter {
public:
    ElasticsearchWriter(const std::string& host, int port);
    ~ElasticsearchWriter();

    // Get existing document
    nlohmann::json getDocument(const std::string& index, const std::string& id);
    
    // Upsert document with retry and exponential backoff
    bool upsertDocument(const std::string& index, const std::string& id, 
                        const nlohmann::json& document, int max_retries = 3);
    
    // Delete document
    bool deleteDocument(const std::string& index, const std::string& id);

private:
    std::string host_;
    int port_;
    std::string base_url_;
    
    std::string performRequest(const std::string& url, const std::string& method,
                                const std::string& data = "");
};

class RedisClient {
public:
    RedisClient(const std::string& host, int port);
    ~RedisClient();

    // Set cache entry
    bool set(const std::string& key, const std::string& value);
    
    // Delete cache entry
    bool del(const std::string& key);
    
    // Get cache entry
    std::string get(const std::string& key);

private:
    std::string host_;
    int port_;
    void* context_; // Redis context (using void* to avoid exposing hiredis in header)
};

class ProductEventConsumer {
public:
    ProductEventConsumer(const std::string& config_file);
    ~ProductEventConsumer();

    // Start consuming events
    void run();
    
    // Stop consumer
    void stop();

private:
    std::unique_ptr<RdKafka::KafkaConsumer> consumer_;
    std::unique_ptr<ElasticsearchWriter> es_writer_;
    std::unique_ptr<RedisClient> redis_client_;
    std::unique_ptr<RdKafka::Producer> dlq_producer_;
    
    std::string topic_;
    std::string dlq_topic_;
    bool running_;
    
    // Process single event
    bool processEvent(const ProductEvent& event);
    
    // Check idempotency (compare version/updated_at)
    bool shouldProcess(const ProductEvent& event, const nlohmann::json& existing_doc);
    
    // Send to DLQ
    void sendToDLQ(const std::string& event_data, const std::string& error_reason);
    
    // Parse event from JSON
    ProductEvent parseEvent(const std::string& json_str);
    
    // Logging and metrics
    void logEvent(const std::string& level, const std::string& message);
    void incrementCounter(const std::string& metric);
};

} // namespace atlas
