#include "search_service.h"
#include <curl/curl.h>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <stdexcept>
#include <iostream>

namespace atlas {

// CURL write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// ElasticsearchClient implementation
ElasticsearchClient::ElasticsearchClient(const std::string& host, int port)
    : host_(host), port_(port) {
    base_url_ = "http://" + host_ + ":" + std::to_string(port_);
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ElasticsearchClient::~ElasticsearchClient() {
    curl_global_cleanup();
}

std::string ElasticsearchClient::performRequest(const std::string& url, const std::string& post_data) {
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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    if (!post_data.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    }

    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    return response_string;
}

nlohmann::json ElasticsearchClient::search(const std::string& query, int size, int timeout_ms) {
    // Build multi_match query with title boosted by 3
    nlohmann::json search_body = {
        {"query", {
            {"multi_match", {
                {"query", query},
                {"fields", {"title^3", "description"}},
                {"type", "best_fields"}
            }}
        }},
        {"size", size},
        {"timeout", std::to_string(timeout_ms) + "ms"}
    };

    std::string url = base_url_ + "/products/_search";
    std::string response = performRequest(url, search_body.dump());
    
    return nlohmann::json::parse(response);
}

// SearchService implementation
SearchService::SearchService(const std::string& es_host, int es_port) {
    es_client_ = std::make_unique<ElasticsearchClient>(es_host, es_port);
}

SearchService::~SearchService() = default;

SearchResponse SearchService::search(const std::string& query, int size) {
    auto start = std::chrono::high_resolution_clock::now();

    SearchResponse response;
    
    try {
        // Query Elasticsearch
        auto es_response = es_client_->search(query, size);
        
        // Parse results
        if (es_response.contains("hits") && es_response["hits"].contains("hits")) {
            auto hits = es_response["hits"]["hits"];
            response.total = es_response["hits"]["total"]["value"];

            for (const auto& hit : hits) {
                SearchResult result;
                result.id = hit["_id"];
                result.es_score = hit["_score"];
                
                auto source = hit["_source"];
                result.title = source.value("title", "");
                result.description = source.value("description", "");
                result.updated_at = source.value("updated_at", "");

                // Apply reranking
                result.score = calculateRerankedScore(
                    result.es_score, 
                    result.updated_at, 
                    result.title, 
                    query
                );
                
                result.recency_score = calculateRecencyScore(result.updated_at);
                result.title_match_score = calculateTitleMatchScore(result.title, query);

                response.results.push_back(result);
            }

            // Sort by reranked score
            std::sort(response.results.begin(), response.results.end(),
                [](const SearchResult& a, const SearchResult& b) {
                    return a.score > b.score;
                });
        }
    } catch (const std::exception& e) {
        std::cerr << "Search error: " << e.what() << std::endl;
        response.total = 0;
    }

    auto end = std::chrono::high_resolution_clock::now();
    response.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return response;
}

double SearchService::calculateRerankedScore(double es_score, const std::string& updated_at,
                                              const std::string& title, const std::string& query) {
    double recency = calculateRecencyScore(updated_at);
    double title_match = calculateTitleMatchScore(title, query);
    
    // Reranking formula: 0.7 * es_score + 0.2 * recency + 0.1 * title_match
    return 0.7 * es_score + 0.2 * recency + 0.1 * title_match;
}

double SearchService::calculateRecencyScore(const std::string& updated_at) {
    if (updated_at.empty()) {
        return 0.0;
    }

    try {
        // Parse ISO 8601 timestamp
        std::tm tm = {};
        std::istringstream ss(updated_at);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        
        auto updated_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto now = std::chrono::system_clock::now();
        auto days_old = std::chrono::duration_cast<std::chrono::hours>(now - updated_time).count() / 24;

        // Exponential decay: score = e^(-days/30)
        // Recent items get higher scores
        double score = std::exp(-days_old / 30.0);
        return std::min(1.0, std::max(0.0, score));
    } catch (...) {
        return 0.5; // Default score on parse error
    }
}

double SearchService::calculateTitleMatchScore(const std::string& title, const std::string& query) {
    if (title.empty() || query.empty()) {
        return 0.0;
    }

    // Convert to lowercase for case-insensitive matching
    std::string title_lower = title;
    std::string query_lower = query;
    std::transform(title_lower.begin(), title_lower.end(), title_lower.begin(), ::tolower);
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

    // Check for exact match
    if (title_lower.find(query_lower) != std::string::npos) {
        return 1.0;
    }

    // Check for partial word matches
    std::istringstream query_stream(query_lower);
    std::string word;
    int matches = 0;
    int total_words = 0;

    while (query_stream >> word) {
        total_words++;
        if (title_lower.find(word) != std::string::npos) {
            matches++;
        }
    }

    return total_words > 0 ? static_cast<double>(matches) / total_words : 0.0;
}

} // namespace atlas
