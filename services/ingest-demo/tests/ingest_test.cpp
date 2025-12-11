#include <gtest/gtest.h>
#include "ingest_server.h"
#include <filesystem>

using namespace atlas;
namespace fs = std::filesystem;

class IngestTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_dir_ = "./test-append-log";
        fs::create_directories(test_log_dir_);
    }

    void TearDown() override {
        // Cleanup test directory
        if (fs::exists(test_log_dir_)) {
            fs::remove_all(test_log_dir_);
        }
    }

    std::string test_log_dir_;
};

TEST_F(IngestTest, AppendLogWriteAndRead) {
    AppendLog log(test_log_dir_);

    std::vector<Event> events;
    for (int i = 0; i < 5; ++i) {
        Event e;
        e.id = "evt-" + std::to_string(i);
        e.type = "test";
        e.data = {{"value", i}};
        e.timestamp = 1234567890 + i;
        events.push_back(e);
    }

    std::string filename = log.writeBatch(events);
    EXPECT_FALSE(filename.empty());

    auto read_events = log.readBatch(filename);
    EXPECT_EQ(read_events.size(), events.size());

    for (size_t i = 0; i < events.size(); ++i) {
        EXPECT_EQ(read_events[i].id, events[i].id);
        EXPECT_EQ(read_events[i].type, events[i].type);
    }
}

TEST_F(IngestTest, AppendLogDeleteBatch) {
    AppendLog log(test_log_dir_);

    std::vector<Event> events;
    Event e;
    e.id = "evt-1";
    e.type = "test";
    e.data = {{"value", 1}};
    e.timestamp = 1234567890;
    events.push_back(e);

    std::string filename = log.writeBatch(events);
    EXPECT_TRUE(fs::exists(test_log_dir_ + "/" + filename));

    log.deleteBatch(filename);
    EXPECT_FALSE(fs::exists(test_log_dir_ + "/" + filename));
}

TEST_F(IngestTest, AppendLogGetPendingBatches) {
    AppendLog log(test_log_dir_);

    // Write multiple batches
    for (int i = 0; i < 3; ++i) {
        std::vector<Event> events;
        Event e;
        e.id = "evt-" + std::to_string(i);
        e.type = "test";
        e.data = {{"value", i}};
        e.timestamp = 1234567890 + i;
        events.push_back(e);
        
        log.writeBatch(events);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Ensure different timestamps
    }

    auto pending = log.getPendingBatches();
    EXPECT_EQ(pending.size(), 3);
}

TEST_F(IngestTest, SinkAPISuccess) {
    SinkAPI sink(0.0); // 0% failure rate

    std::vector<Event> events;
    Event e;
    e.id = "evt-1";
    e.type = "test";
    e.data = {{"value", 1}};
    e.timestamp = 1234567890;
    events.push_back(e);

    bool result = sink.sendBatch(events);
    EXPECT_TRUE(result);
}

TEST_F(IngestTest, SinkAPIFailure) {
    SinkAPI sink(1.0); // 100% failure rate

    std::vector<Event> events;
    Event e;
    e.id = "evt-1";
    e.type = "test";
    e.data = {{"value", 1}};
    e.timestamp = 1234567890;
    events.push_back(e);

    bool result = sink.sendBatch(events);
    EXPECT_FALSE(result);
}

TEST_F(IngestTest, BatcherFlushOnSize) {
    auto log = std::make_shared<AppendLog>(test_log_dir_);
    auto sink = std::make_shared<SinkAPI>(0.0);
    
    Batcher batcher(5, 10000, log, sink); // Batch size 5, 10 second timeout
    batcher.start();

    // Add 5 events to trigger flush
    for (int i = 0; i < 5; ++i) {
        Event e;
        e.id = "evt-" + std::to_string(i);
        e.type = "test";
        e.data = {{"value", i}};
        e.timestamp = 1234567890 + i;
        batcher.addEvent(e);
    }

    // Wait for flush
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    batcher.stop();

    // Check that batch was processed (no pending files since sink succeeds)
    auto pending = log->getPendingBatches();
    EXPECT_EQ(pending.size(), 0);
}

TEST_F(IngestTest, ReplayPendingBatches) {
    // Create some pending batches
    auto log = std::make_shared<AppendLog>(test_log_dir_);
    auto sink = std::make_shared<SinkAPI>(1.0); // 100% failure to create pending files

    std::vector<Event> events;
    for (int i = 0; i < 3; ++i) {
        Event e;
        e.id = "evt-" + std::to_string(i);
        e.type = "test";
        e.data = {{"value", i}};
        e.timestamp = 1234567890 + i;
        events.push_back(e);
    }

    log->writeBatch(events);

    // Now create server with successful sink
    sink->setFailureRate(0.0);
    
    IngestionServer server(8081, 1000, 10, 1000, test_log_dir_);
    server.replayPendingBatches();

    // Check that pending batches were processed
    auto pending = log->getPendingBatches();
    EXPECT_EQ(pending.size(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
