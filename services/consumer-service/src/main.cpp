#include "consumer.h"
#include <iostream>
#include <csignal>

std::unique_ptr<atlas::ProductEventConsumer> g_consumer;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    if (g_consumer) {
        g_consumer->stop();
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.yml>" << std::endl;
        return 1;
    }

    std::string config_file = argv[1];

    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        std::cout << "Starting Product Event Consumer..." << std::endl;
        std::cout << "Config file: " << config_file << std::endl;

        g_consumer = std::make_unique<atlas::ProductEventConsumer>(config_file);
        g_consumer->run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Consumer shutdown complete" << std::endl;
    return 0;
}
