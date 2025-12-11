#include "ingest_server.h"
#include <httplib.h>
#include <iostream>
#include <csignal>

std::unique_ptr<atlas::IngestionServer> g_server;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        std::cout << "Starting Ingestion Demo Server..." << std::endl;

        // Configuration
        int port = 8081;
        int queue_size = 10000;
        int batch_size = 100;
        int batch_wait_ms = 1000; // 1 second
        std::string log_dir = "./append-log";

        g_server = std::make_unique<atlas::IngestionServer>(
            port, queue_size, batch_size, batch_wait_ms, log_dir
        );

        g_server->start();

        // Create HTTP server
        httplib::Server http_server;

        // POST /events endpoint
        http_server.Post("/events", [](const httplib::Request& req, httplib::Response& res) {
            std::string response;
            int status_code;
            
            g_server->handlePostEvent(req.body, response, status_code);
            
            res.status = status_code;
            res.set_content(response, "application/json");
        });

        // Health check
        http_server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            nlohmann::json response = {
                {"status", "healthy"},
                {"service", "ingest-demo"}
            };
            res.set_content(response.dump(), "application/json");
        });

        std::cout << "HTTP server listening on http://localhost:" << port << std::endl;
        std::cout << "Endpoints:" << std::endl;
        std::cout << "  POST /events" << std::endl;
        std::cout << "  GET /health" << std::endl;

        http_server.listen("0.0.0.0", port);

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}
