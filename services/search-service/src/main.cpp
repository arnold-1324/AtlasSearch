#include "search_service.h"
#include <httplib.h>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    std::cout << "Starting AtlasSearch Service..." << std::endl;

    // Initialize search service
    atlas::SearchService search_service("localhost", 9200);

    // Create HTTP server
    httplib::Server server;

    // Health check endpoint
    server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        json response = {
            {"status", "healthy"},
            {"service", "atlas-search"},
            {"version", "1.0.0"}
        };
        res.set_content(response.dump(), "application/json");
    });

    // Search endpoint: GET /search?q=term&size=10
    server.Get("/search", [&search_service](const httplib::Request& req, httplib::Response& res) {
        // Extract query parameters
        std::string query = req.has_param("q") ? req.get_param_value("q") : "";
        int size = req.has_param("size") ? std::stoi(req.get_param_value("size")) : 10;

        if (query.empty()) {
            json error_response = {
                {"error", "Missing required parameter 'q'"},
                {"status", 400}
            };
            res.status = 400;
            res.set_content(error_response.dump(), "application/json");
            return;
        }

        // Validate size
        if (size < 1 || size > 100) {
            size = 10;
        }

        try {
            // Perform search
            auto search_response = search_service.search(query, size);

            // Build JSON response
            json results_json = json::array();
            for (const auto& result : search_response.results) {
                results_json.push_back({
                    {"id", result.id},
                    {"title", result.title},
                    {"description", result.description},
                    {"score", result.score},
                    {"es_score", result.es_score},
                    {"recency_score", result.recency_score},
                    {"title_match_score", result.title_match_score},
                    {"updated_at", result.updated_at}
                });
            }

            json response = {
                {"results", results_json},
                {"total", search_response.total},
                {"latency_ms", search_response.latency_ms},
                {"query", query},
                {"size", size}
            };

            res.set_content(response.dump(2), "application/json");
            std::cout << "Search query: '" << query << "' - " 
                      << search_response.results.size() << " results in " 
                      << search_response.latency_ms << "ms" << std::endl;

        } catch (const std::exception& e) {
            json error_response = {
                {"error", e.what()},
                {"status", 500}
            };
            res.status = 500;
            res.set_content(error_response.dump(), "application/json");
            std::cerr << "Search error: " << e.what() << std::endl;
        }
    });

    // Start server
    std::cout << "Server listening on http://localhost:8080" << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  GET /health" << std::endl;
    std::cout << "  GET /search?q=<query>&size=<size>" << std::endl;

    server.listen("0.0.0.0", 8080);

    return 0;
}
