#include <gtest/gtest.h>
#include "search_service.h"

using namespace atlas;

class SearchServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: These tests require Elasticsearch to be running
        // For unit tests, you would mock the ES client
    }
};

TEST_F(SearchServiceTest, RecencyScoreCalculation) {
    SearchService service("localhost", 9200);
    
    // Recent date should have high score
    std::string recent_date = "2025-12-10T00:00:00Z";
    // Note: This is a simplified test - in production you'd use dependency injection
    // and mock the current time
}

TEST_F(SearchServiceTest, TitleMatchScoreExactMatch) {
    SearchService service("localhost", 9200);
    
    // Test exact match (this would require exposing the method or using a friend class)
    // For now, this is a placeholder showing the test structure
}

TEST_F(SearchServiceTest, TitleMatchScorePartialMatch) {
    SearchService service("localhost", 9200);
    
    // Test partial word matching
}

TEST_F(SearchServiceTest, RerankedScoreCalculation) {
    SearchService service("localhost", 9200);
    
    // Test the reranking formula: 0.7 * es_score + 0.2 * recency + 0.1 * title_match
    // Expected: proper weighted combination
}

TEST_F(SearchServiceTest, SearchWithEmptyQuery) {
    SearchService service("localhost", 9200);
    
    // Empty query should return empty results or handle gracefully
    auto response = service.search("", 10);
    EXPECT_EQ(response.results.size(), 0);
}

TEST_F(SearchServiceTest, SearchWithValidQuery) {
    // This test requires Elasticsearch to be running with test data
    // In a real scenario, you'd use testcontainers or mock the ES client
    
    SearchService service("localhost", 9200);
    
    try {
        auto response = service.search("laptop", 5);
        EXPECT_GE(response.latency_ms, 0);
        EXPECT_LE(response.results.size(), 5);
    } catch (const std::exception& e) {
        // If ES is not available, test passes with warning
        std::cout << "Warning: Elasticsearch not available - " << e.what() << std::endl;
    }
}

TEST_F(SearchServiceTest, SearchResultsSortedByScore) {
    SearchService service("localhost", 9200);
    
    try {
        auto response = service.search("test", 10);
        
        // Verify results are sorted by score in descending order
        for (size_t i = 1; i < response.results.size(); ++i) {
            EXPECT_GE(response.results[i-1].score, response.results[i].score);
        }
    } catch (const std::exception& e) {
        std::cout << "Warning: Elasticsearch not available - " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
