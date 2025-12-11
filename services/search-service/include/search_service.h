#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace atlas {

struct SearchResult {
    std::string id;
    std::string title;
    std::string description;
    double score;
    double es_score;
    double recency_score;
    double title_match_score;
    std::string updated_at;
};

struct SearchResponse {
    std::vector<SearchResult> results;
    int total;
    int latency_ms;
};

class ElasticsearchClient {
public:
    ElasticsearchClient(const std::string& host, int port);
    ~ElasticsearchClient();

    // Perform multi_match search
    nlohmann::json search(const std::string& query, int size, int timeout_ms = 5000);

private:
    std::string host_;
    int port_;
    std::string base_url_;
    
    // HTTP request helper
    std::string performRequest(const std::string& url, const std::string& post_data = "");
};

class SearchService {
public:
    SearchService(const std::string& es_host, int es_port);
    ~SearchService();

    // Main search endpoint
    SearchResponse search(const std::string& query, int size = 10);

private:
    std::unique_ptr<ElasticsearchClient> es_client_;

    // Reranking: score = 0.7 * es_score + 0.2 * recency + 0.1 * title_match
    double calculateRerankedScore(double es_score, const std::string& updated_at, 
                                   const std::string& title, const std::string& query);
    
    double calculateRecencyScore(const std::string& updated_at);
    double calculateTitleMatchScore(const std::string& title, const std::string& query);
};

} // namespace atlas
