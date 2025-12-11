#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstring>

std::atomic<int> sent_count{0};
std::atomic<int> success_count{0};
std::atomic<int> error_count{0};
std::atomic<int> backpressure_count{0};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool sendEvent(const std::string& url, const std::string& event_json) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, event_json.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    sent_count++;

    if (res == CURLE_OK) {
        if (http_code == 202) {
            success_count++;
            return true;
        } else if (http_code == 429) {
            backpressure_count++;
            return false;
        } else {
            error_count++;
            return false;
        }
    } else {
        error_count++;
        return false;
    }
}

void producerWorker(const std::string& url, int rate_per_sec, int total_events, int worker_id) {
    int events_to_send = total_events;
    int delay_ms = 1000 / rate_per_sec;

    for (int i = 0; i < events_to_send; ++i) {
        nlohmann::json event = {
            {"id", "evt-" + std::to_string(worker_id) + "-" + std::to_string(i)},
            {"type", "test"},
            {"data", {
                {"message", "Test event from producer"},
                {"worker_id", worker_id},
                {"sequence", i}
            }}
        };

        sendEvent(url, event.dump());

        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
}

int main(int argc, char** argv) {
    std::string url = "http://localhost:8081/events";
    int rate = 100;  // events per second
    int burst = 500; // total events
    int workers = 1;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--url") == 0 && i + 1 < argc) {
            url = argv[++i];
        } else if (std::strcmp(argv[i], "--rate") == 0 && i + 1 < argc) {
            rate = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--burst") == 0 && i + 1 < argc) {
            burst = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--workers") == 0 && i + 1 < argc) {
            workers = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --url <url>        Target URL (default: http://localhost:8081/events)\n"
                      << "  --rate <n>         Events per second per worker (default: 100)\n"
                      << "  --burst <n>        Total events to send (default: 500)\n"
                      << "  --workers <n>      Number of worker threads (default: 1)\n"
                      << "  --help             Show this help message\n";
            return 0;
        }
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cout << "Producer Tool Configuration:" << std::endl;
    std::cout << "  URL: " << url << std::endl;
    std::cout << "  Rate: " << rate << " events/sec/worker" << std::endl;
    std::cout << "  Burst: " << burst << " total events" << std::endl;
    std::cout << "  Workers: " << workers << std::endl;
    std::cout << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // Launch worker threads
    std::vector<std::thread> threads;
    int events_per_worker = burst / workers;

    for (int i = 0; i < workers; ++i) {
        threads.emplace_back(producerWorker, url, rate, events_per_worker, i);
    }

    // Wait for all workers to complete
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    curl_global_cleanup();

    // Print statistics
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Producer Statistics" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total sent: " << sent_count << std::endl;
    std::cout << "Accepted (202): " << success_count << std::endl;
    std::cout << "Backpressure (429): " << backpressure_count << std::endl;
    std::cout << "Errors: " << error_count << std::endl;
    std::cout << "Duration: " << duration << " ms" << std::endl;
    std::cout << "Throughput: " << (sent_count * 1000.0 / duration) << " events/sec" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
