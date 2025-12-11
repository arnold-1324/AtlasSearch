#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include <boost/lockfree/queue.hpp>

namespace atlas {

struct Event {
    std::string id;
    std::string type;
    nlohmann::json data;
    int64_t timestamp;
};

// Append-only log for durability
class AppendLog {
public:
    AppendLog(const std::string& log_dir);
    ~AppendLog();

    // Write batch to log file
    std::string writeBatch(const std::vector<Event>& events);
    
    // Delete log file after successful sink
    void deleteBatch(const std::string& filename);
    
    // Get all pending batch files
    std::vector<std::string> getPendingBatches();
    
    // Read events from batch file
    std::vector<Event> readBatch(const std::string& filename);

private:
    std::string log_dir_;
    std::mutex mutex_;
    int batch_counter_;
};

// Simulated sink API
class SinkAPI {
public:
    SinkAPI(double failure_rate = 0.0);
    
    // Send batch to sink (returns true on success)
    bool sendBatch(const std::vector<Event>& events);
    
    void setFailureRate(double rate) { failure_rate_ = rate; }

private:
    double failure_rate_;
};

// Background batcher
class Batcher {
public:
    Batcher(int max_batch_size, int max_wait_ms, 
            std::shared_ptr<AppendLog> log,
            std::shared_ptr<SinkAPI> sink);
    ~Batcher();

    // Add event to batch
    void addEvent(const Event& event);
    
    // Start background thread
    void start();
    
    // Stop background thread
    void stop();

private:
    int max_batch_size_;
    int max_wait_ms_;
    std::shared_ptr<AppendLog> log_;
    std::shared_ptr<SinkAPI> sink_;
    
    std::vector<Event> current_batch_;
    std::mutex batch_mutex_;
    std::condition_variable batch_cv_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> worker_thread_;
    
    void workerLoop();
    void flushBatch();
};

// Ingestion server
class IngestionServer {
public:
    IngestionServer(int port, int queue_size, int batch_size, int batch_wait_ms,
                    const std::string& log_dir);
    ~IngestionServer();

    // Start server
    void start();
    
    // Stop server
    void stop();
    
    // Replay pending batches on startup
    void replayPendingBatches();

private:
    int port_;
    int queue_size_;
    
    std::shared_ptr<AppendLog> log_;
    std::shared_ptr<SinkAPI> sink_;
    std::shared_ptr<Batcher> batcher_;
    
    // Lock-free bounded queue
    std::unique_ptr<boost::lockfree::queue<Event*>> event_queue_;
    
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> consumer_thread_;
    
    void consumerLoop();
    void handlePostEvent(const std::string& body, std::string& response, int& status_code);
};

} // namespace atlas
