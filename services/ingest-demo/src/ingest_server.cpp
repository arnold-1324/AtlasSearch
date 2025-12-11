#include "ingest_server.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <random>
#include <iomanip>

namespace fs = std::filesystem;

namespace atlas {

// AppendLog implementation
AppendLog::AppendLog(const std::string& log_dir)
    : log_dir_(log_dir), batch_counter_(0) {
    
    // Create log directory if it doesn't exist
    fs::create_directories(log_dir_);
    
    std::cout << "AppendLog initialized: " << log_dir_ << std::endl;
}

AppendLog::~AppendLog() = default;

std::string AppendLog::writeBatch(const std::vector<Event>& events) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Generate filename with timestamp and counter
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream filename_stream;
    filename_stream << "batch_"
                    << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
                    << "_" << batch_counter_++ << ".jsonl";
    
    std::string filename = filename_stream.str();
    std::string filepath = log_dir_ + "/" + filename;
    
    // Write events as JSONL (one JSON per line)
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filepath);
    }
    
    for (const auto& event : events) {
        nlohmann::json j = {
            {"id", event.id},
            {"type", event.type},
            {"data", event.data},
            {"timestamp", event.timestamp}
        };
        file << j.dump() << "\n";
    }
    
    file.close();
    
    std::cout << "Wrote batch to: " << filename << " (" << events.size() << " events)" << std::endl;
    
    return filename;
}

void AppendLog::deleteBatch(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string filepath = log_dir_ + "/" + filename;
    
    if (fs::exists(filepath)) {
        fs::remove(filepath);
        std::cout << "Deleted batch file: " << filename << std::endl;
    }
}

std::vector<std::string> AppendLog::getPendingBatches() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> batches;
    
    for (const auto& entry : fs::directory_iterator(log_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".jsonl") {
            batches.push_back(entry.path().filename().string());
        }
    }
    
    // Sort by filename (chronological order)
    std::sort(batches.begin(), batches.end());
    
    return batches;
}

std::vector<Event> AppendLog::readBatch(const std::string& filename) {
    std::string filepath = log_dir_ + "/" + filename;
    
    std::vector<Event> events;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open batch file: " + filepath);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        nlohmann::json j = nlohmann::json::parse(line);
        
        Event event;
        event.id = j["id"];
        event.type = j["type"];
        event.data = j["data"];
        event.timestamp = j["timestamp"];
        
        events.push_back(event);
    }
    
    file.close();
    
    return events;
}

// SinkAPI implementation
SinkAPI::SinkAPI(double failure_rate)
    : failure_rate_(failure_rate) {
}

bool SinkAPI::sendBatch(const std::vector<Event>& events) {
    // Simulate network delay
    std::this_thread::sleep_for(std::chrono::milliseconds(10 + rand() % 40));
    
    // Simulate random failures
    if (failure_rate_ > 0.0) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        
        if (dis(gen) < failure_rate_) {
            std::cerr << "SinkAPI: Simulated failure" << std::endl;
            return false;
        }
    }
    
    std::cout << "SinkAPI: Successfully sent batch (" << events.size() << " events)" << std::endl;
    return true;
}

// Batcher implementation
Batcher::Batcher(int max_batch_size, int max_wait_ms,
                 std::shared_ptr<AppendLog> log,
                 std::shared_ptr<SinkAPI> sink)
    : max_batch_size_(max_batch_size),
      max_wait_ms_(max_wait_ms),
      log_(log),
      sink_(sink),
      running_(false) {
}

Batcher::~Batcher() {
    stop();
}

void Batcher::addEvent(const Event& event) {
    std::lock_guard<std::mutex> lock(batch_mutex_);
    current_batch_.push_back(event);
    
    if (current_batch_.size() >= static_cast<size_t>(max_batch_size_)) {
        batch_cv_.notify_one();
    }
}

void Batcher::start() {
    running_ = true;
    worker_thread_ = std::make_unique<std::thread>(&Batcher::workerLoop, this);
}

void Batcher::stop() {
    running_ = false;
    batch_cv_.notify_one();
    
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    
    // Flush remaining events
    if (!current_batch_.empty()) {
        flushBatch();
    }
}

void Batcher::workerLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(batch_mutex_);
        
        // Wait for batch to fill or timeout
        batch_cv_.wait_for(lock, std::chrono::milliseconds(max_wait_ms_), [this] {
            return current_batch_.size() >= static_cast<size_t>(max_batch_size_) || !running_;
        });
        
        if (!current_batch_.empty()) {
            flushBatch();
        }
    }
}

void Batcher::flushBatch() {
    if (current_batch_.empty()) return;
    
    std::vector<Event> batch_to_send;
    {
        std::lock_guard<std::mutex> lock(batch_mutex_);
        batch_to_send = std::move(current_batch_);
        current_batch_.clear();
    }
    
    // Step 1: Write to append log
    std::string filename = log_->writeBatch(batch_to_send);
    
    // Step 2: Send to sink
    bool success = sink_->sendBatch(batch_to_send);
    
    // Step 3: Delete log file on success
    if (success) {
        log_->deleteBatch(filename);
    } else {
        std::cerr << "Batch failed to send, keeping log file: " << filename << std::endl;
    }
}

// IngestionServer implementation
IngestionServer::IngestionServer(int port, int queue_size, int batch_size, int batch_wait_ms,
                                 const std::string& log_dir)
    : port_(port),
      queue_size_(queue_size),
      running_(false) {
    
    log_ = std::make_shared<AppendLog>(log_dir);
    sink_ = std::make_shared<SinkAPI>(0.0); // 0% failure rate by default
    batcher_ = std::make_shared<Batcher>(batch_size, batch_wait_ms, log_, sink_);
    
    event_queue_ = std::make_unique<boost::lockfree::queue<Event*>>(queue_size);
    
    std::cout << "IngestionServer initialized (port=" << port_ 
              << ", queue_size=" << queue_size_ << ")" << std::endl;
}

IngestionServer::~IngestionServer() {
    stop();
}

void IngestionServer::replayPendingBatches() {
    std::cout << "Replaying pending batches..." << std::endl;
    
    auto pending = log_->getPendingBatches();
    
    if (pending.empty()) {
        std::cout << "No pending batches to replay" << std::endl;
        return;
    }
    
    std::cout << "Found " << pending.size() << " pending batches" << std::endl;
    
    for (const auto& filename : pending) {
        try {
            std::vector<Event> events = log_->readBatch(filename);
            std::cout << "Replaying batch: " << filename << " (" << events.size() << " events)" << std::endl;
            
            bool success = sink_->sendBatch(events);
            
            if (success) {
                log_->deleteBatch(filename);
                std::cout << "Successfully replayed and deleted: " << filename << std::endl;
            } else {
                std::cerr << "Failed to replay batch: " << filename << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error replaying batch " << filename << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "Replay complete" << std::endl;
}

void IngestionServer::start() {
    // Replay pending batches before accepting new traffic
    replayPendingBatches();
    
    running_ = true;
    
    // Start batcher
    batcher_->start();
    
    // Start consumer thread
    consumer_thread_ = std::make_unique<std::thread>(&IngestionServer::consumerLoop, this);
    
    std::cout << "IngestionServer started on port " << port_ << std::endl;
}

void IngestionServer::stop() {
    running_ = false;
    
    if (consumer_thread_ && consumer_thread_->joinable()) {
        consumer_thread_->join();
    }
    
    batcher_->stop();
    
    // Drain queue
    Event* event;
    while (event_queue_->pop(event)) {
        batcher_->addEvent(*event);
        delete event;
    }
    
    std::cout << "IngestionServer stopped" << std::endl;
}

void IngestionServer::consumerLoop() {
    while (running_) {
        Event* event;
        if (event_queue_->pop(event)) {
            batcher_->addEvent(*event);
            delete event;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void IngestionServer::handlePostEvent(const std::string& body, std::string& response, int& status_code) {
    try {
        nlohmann::json j = nlohmann::json::parse(body);
        
        Event* event = new Event();
        event->id = j.value("id", "");
        event->type = j.value("type", "");
        event->data = j.value("data", nlohmann::json::object());
        event->timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Try to push to queue
        if (event_queue_->push(event)) {
            response = R"({"status": "accepted"})";
            status_code = 202;
        } else {
            // Queue is full - backpressure
            delete event;
            response = R"({"error": "Queue full, please retry later"})";
            status_code = 429;
        }
        
    } catch (const std::exception& e) {
        response = R"({"error": "Invalid JSON"})";
        status_code = 400;
    }
}

} // namespace atlas
