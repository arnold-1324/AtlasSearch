#include "consumer.h"
#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <yaml-cpp/yaml.h>

namespace atlas {

// CURL callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ElasticsearchWriter implementation
ElasticsearchWriter::ElasticsearchWriter(const std::string& host, int port)
    : host_(host), port_(port) {
    base_url_ = "http://" + host_ + ":" + std::to_string(port_);
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ElasticsearchWriter::~ElasticsearchWriter() {
    curl_global_cleanup();
}

std::string ElasticsearchWriter::performRequest(const std::string& url, const std::string& method,
                                                 const std::string& data) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    std::string response_string;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    } else if (method == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    if (http_code >= 400) {
        throw std::runtime_error("HTTP error " + std::to_string(http_code) + ": " + response_string);
    }

    return response_string;
}

nlohmann::json ElasticsearchWriter::getDocument(const std::string& index, const std::string& id) {
    try {
        std::string url = base_url_ + "/" + index + "/_doc/" + id;
        std::string response = performRequest(url, "GET");
        return nlohmann::json::parse(response);
    } catch (const std::exception& e) {
        // Document doesn't exist or error occurred
        return nlohmann::json::object();
    }
}

bool ElasticsearchWriter::upsertDocument(const std::string& index, const std::string& id,
                                         const nlohmann::json& document, int max_retries) {
    int attempt = 0;
    
    while (attempt < max_retries) {
        try {
            std::string url = base_url_ + "/" + index + "/_doc/" + id;
            performRequest(url, "PUT", document.dump());
            return true;
        } catch (const std::exception& e) {
            attempt++;
            if (attempt >= max_retries) {
                std::cerr << "Failed to upsert document after " << max_retries 
                          << " attempts: " << e.what() << std::endl;
                return false;
            }
            
            // Exponential backoff: 100ms, 200ms, 400ms, ...
            int backoff_ms = 100 * std::pow(2, attempt - 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        }
    }
    
    return false;
}

bool ElasticsearchWriter::deleteDocument(const std::string& index, const std::string& id) {
    try {
        std::string url = base_url_ + "/" + index + "/_doc/" + id;
        performRequest(url, "DELETE");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to delete document: " << e.what() << std::endl;
        return false;
    }
}

// RedisClient implementation
RedisClient::RedisClient(const std::string& host, int port)
    : host_(host), port_(port) {
    redisContext* c = redisConnect(host.c_str(), port);
    if (c == nullptr || c->err) {
        if (c) {
            std::cerr << "Redis connection error: " << c->errstr << std::endl;
            redisFree(c);
        }
        throw std::runtime_error("Failed to connect to Redis");
    }
    context_ = c;
}

RedisClient::~RedisClient() {
    if (context_) {
        redisFree(static_cast<redisContext*>(context_));
    }
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    redisContext* c = static_cast<redisContext*>(context_);
    redisReply* reply = (redisReply*)redisCommand(c, "SET %s %s", key.c_str(), value.c_str());
    
    if (reply == nullptr) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS && 
                    std::string(reply->str) == "OK");
    freeReplyObject(reply);
    return success;
}

bool RedisClient::del(const std::string& key) {
    redisContext* c = static_cast<redisContext*>(context_);
    redisReply* reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
    
    if (reply == nullptr) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_INTEGER);
    freeReplyObject(reply);
    return success;
}

std::string RedisClient::get(const std::string& key) {
    redisContext* c = static_cast<redisContext*>(context_);
    redisReply* reply = (redisReply*)redisCommand(c, "GET %s", key.c_str());
    
    if (reply == nullptr || reply->type != REDIS_REPLY_STRING) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    
    std::string value(reply->str);
    freeReplyObject(reply);
    return value;
}

// ProductEventConsumer implementation
ProductEventConsumer::ProductEventConsumer(const std::string& config_file)
    : running_(false) {
    
    // Load configuration
    YAML::Node config = YAML::LoadFile(config_file);
    
    std::string kafka_brokers = config["kafka"]["brokers"].as<std::string>();
    std::string group_id = config["kafka"]["group_id"].as<std::string>();
    topic_ = config["kafka"]["topic"].as<std::string>();
    dlq_topic_ = config["kafka"]["dlq_topic"].as<std::string>();
    
    std::string es_host = config["elasticsearch"]["host"].as<std::string>();
    int es_port = config["elasticsearch"]["port"].as<int>();
    
    std::string redis_host = config["redis"]["host"].as<std::string>();
    int redis_port = config["redis"]["port"].as<int>();
    
    // Initialize Kafka consumer
    std::string errstr;
    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    
    conf->set("bootstrap.servers", kafka_brokers, errstr);
    conf->set("group.id", group_id, errstr);
    conf->set("enable.auto.commit", "false", errstr); // Manual commit
    conf->set("auto.offset.reset", "earliest", errstr);
    
    consumer_.reset(RdKafka::KafkaConsumer::create(conf, errstr));
    if (!consumer_) {
        throw std::runtime_error("Failed to create Kafka consumer: " + errstr);
    }
    
    delete conf;
    
    // Subscribe to topic
    std::vector<std::string> topics = {topic_};
    RdKafka::ErrorCode err = consumer_->subscribe(topics);
    if (err) {
        throw std::runtime_error("Failed to subscribe to topic: " + RdKafka::err2str(err));
    }
    
    // Initialize DLQ producer
    RdKafka::Conf* producer_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    producer_conf->set("bootstrap.servers", kafka_brokers, errstr);
    
    dlq_producer_.reset(RdKafka::Producer::create(producer_conf, errstr));
    delete producer_conf;
    
    // Initialize ES and Redis clients
    es_writer_ = std::make_unique<ElasticsearchWriter>(es_host, es_port);
    redis_client_ = std::make_unique<RedisClient>(redis_host, redis_port);
    
    logEvent("INFO", "Consumer initialized successfully");
}

ProductEventConsumer::~ProductEventConsumer() {
    stop();
}

void ProductEventConsumer::run() {
    running_ = true;
    logEvent("INFO", "Starting consumer loop");
    
    while (running_) {
        RdKafka::Message* msg = consumer_->consume(1000); // 1 second timeout
        
        if (msg->err() == RdKafka::ERR_NO_ERROR) {
            std::string payload(static_cast<const char*>(msg->payload()), msg->len());
            
            try {
                ProductEvent event = parseEvent(payload);
                bool success = processEvent(event);
                
                if (success) {
                    // Manually commit offset only after successful processing
                    consumer_->commitSync(msg);
                    incrementCounter("events_processed");
                    logEvent("INFO", "Successfully processed event: " + event.event_id);
                } else {
                    // Send to DLQ after repeated failures
                    sendToDLQ(payload, "Processing failed after retries");
                    consumer_->commitSync(msg); // Commit to avoid reprocessing
                    incrementCounter("events_failed");
                }
                
            } catch (const std::exception& e) {
                logEvent("ERROR", "Failed to parse event: " + std::string(e.what()));
                sendToDLQ(payload, "Parse error: " + std::string(e.what()));
                consumer_->commitSync(msg);
                incrementCounter("events_parse_error");
            }
        } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
            logEvent("ERROR", "Kafka error: " + msg->errstr());
        }
        
        delete msg;
    }
    
    logEvent("INFO", "Consumer stopped");
}

void ProductEventConsumer::stop() {
    running_ = false;
    if (consumer_) {
        consumer_->close();
    }
}

bool ProductEventConsumer::processEvent(const ProductEvent& event) {
    try {
        // Step 1: Fetch existing document from ES
        nlohmann::json existing_doc = es_writer_->getDocument("products", event.product_id);
        
        // Step 2: Check idempotency
        if (!shouldProcess(event, existing_doc)) {
            logEvent("INFO", "Skipping event due to idempotency check: " + event.event_id);
            return true; // Not an error, just skip
        }
        
        // Step 3: Process based on event type
        bool es_success = false;
        
        if (event.event_type == "delete") {
            es_success = es_writer_->deleteDocument("products", event.product_id);
        } else {
            // create or update
            nlohmann::json doc = event.data;
            doc["version"] = event.version;
            doc["updated_at"] = event.updated_at;
            doc["product_id"] = event.product_id;
            
            es_success = es_writer_->upsertDocument("products", event.product_id, doc);
        }
        
        if (!es_success) {
            return false;
        }
        
        // Step 4: Update Redis cache
        std::string cache_key = "product:" + event.product_id;
        
        if (event.event_type == "delete") {
            redis_client_->del(cache_key);
        } else {
            // On update failure, delete key to force reload
            if (!redis_client_->set(cache_key, event.data.dump())) {
                redis_client_->del(cache_key);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        logEvent("ERROR", "Error processing event: " + std::string(e.what()));
        return false;
    }
}

bool ProductEventConsumer::shouldProcess(const ProductEvent& event, const nlohmann::json& existing_doc) {
    if (existing_doc.empty() || !existing_doc.contains("_source")) {
        return true; // No existing document, process the event
    }
    
    auto source = existing_doc["_source"];
    
    // Compare version
    if (source.contains("version")) {
        int existing_version = source["version"];
        if (event.version <= existing_version) {
            return false; // Older or same version, skip
        }
    }
    
    // Compare updated_at timestamp
    if (source.contains("updated_at")) {
        std::string existing_updated_at = source["updated_at"];
        if (event.updated_at <= existing_updated_at) {
            return false; // Older or same timestamp, skip
        }
    }
    
    return true;
}

void ProductEventConsumer::sendToDLQ(const std::string& event_data, const std::string& error_reason) {
    nlohmann::json dlq_message = {
        {"original_event", event_data},
        {"error_reason", error_reason},
        {"timestamp", std::time(nullptr)}
    };
    
    std::string payload = dlq_message.dump();
    
    RdKafka::ErrorCode err = dlq_producer_->produce(
        dlq_topic_,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(payload.c_str()),
        payload.size(),
        nullptr, 0,
        nullptr
    );
    
    if (err != RdKafka::ERR_NO_ERROR) {
        logEvent("ERROR", "Failed to send to DLQ: " + RdKafka::err2str(err));
    }
    
    dlq_producer_->poll(0);
}

ProductEvent ProductEventConsumer::parseEvent(const std::string& json_str) {
    nlohmann::json j = nlohmann::json::parse(json_str);
    
    ProductEvent event;
    event.product_id = j["product_id"];
    event.event_id = j["event_id"];
    event.event_type = j["event_type"];
    event.version = j["version"];
    event.updated_at = j["updated_at"];
    event.data = j["data"];
    
    return event;
}

void ProductEventConsumer::logEvent(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << level << "] " << message << std::endl;
}

void ProductEventConsumer::incrementCounter(const std::string& metric) {
    // In production, this would send to Prometheus, StatsD, etc.
    // For now, just log
    static std::map<std::string, int> counters;
    counters[metric]++;
    
    if (counters[metric] % 100 == 0) {
        logEvent("METRICS", metric + ": " + std::to_string(counters[metric]));
    }
}

} // namespace atlas
