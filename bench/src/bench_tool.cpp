#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <curl/curl.h>
#include <cstring>
#include <iomanip>

struct LatencyStats {
    std::vector<double> latencies;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

double performRequest(const std::string& url, LatencyStats& stats, int thread_id) {
    auto start = std::chrono::high_resolution_clock::now();
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        stats.error_count++;
        return -1.0;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    
    auto end = std::chrono::high_resolution_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && http_code == 200) {
        stats.success_count++;
        return latency_ms;
    } else {
        stats.error_count++;
        return -1.0;
    }
}

void workerThread(const std::string& url, int requests_per_thread, 
                  LatencyStats& stats, int thread_id) {
    std::vector<double> local_latencies;
    local_latencies.reserve(requests_per_thread);

    for (int i = 0; i < requests_per_thread; ++i) {
        double latency = performRequest(url, stats, thread_id);
        if (latency > 0) {
            local_latencies.push_back(latency);
        }
    }

    // Merge local latencies into global stats (with lock)
    static std::mutex latency_mutex;
    std::lock_guard<std::mutex> lock(latency_mutex);
    stats.latencies.insert(stats.latencies.end(), 
                           local_latencies.begin(), 
                           local_latencies.end());
}

double percentile(std::vector<double>& data, double p) {
    if (data.empty()) return 0.0;
    
    std::sort(data.begin(), data.end());
    size_t index = static_cast<size_t>(p * data.size());
    if (index >= data.size()) index = data.size() - 1;
    
    return data[index];
}

int main(int argc, char** argv) {
    std::string url = "http://localhost:8080/search?q=test&size=10";
    int concurrency = 10;
    int total_requests = 1000;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--url") == 0 && i + 1 < argc) {
            url = argv[++i];
        } else if (std::strcmp(argv[i], "--concurrency") == 0 && i + 1 < argc) {
            concurrency = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--requests") == 0 && i + 1 < argc) {
            total_requests = std::stoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --url <url>           Target URL (default: http://localhost:8080/search?q=test&size=10)\n"
                      << "  --concurrency <n>     Number of concurrent threads (default: 10)\n"
                      << "  --requests <n>        Total number of requests (default: 1000)\n"
                      << "  --help                Show this help message\n";
            return 0;
        }
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::cout << "========================================" << std::endl;
    std::cout << "AtlasSearch Benchmark Tool" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "URL: " << url << std::endl;
    std::cout << "Concurrency: " << concurrency << std::endl;
    std::cout << "Total Requests: " << total_requests << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    LatencyStats stats;
    stats.latencies.reserve(total_requests);

    int requests_per_thread = total_requests / concurrency;
    
    auto start = std::chrono::high_resolution_clock::now();

    // Launch worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < concurrency; ++i) {
        threads.emplace_back(workerThread, url, requests_per_thread, 
                            std::ref(stats), i);
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration<double>(end - start).count();

    curl_global_cleanup();

    // Calculate statistics
    int total_completed = stats.success_count + stats.error_count;
    double success_rate = (total_completed > 0) 
        ? (100.0 * stats.success_count / total_completed) 
        : 0.0;
    double throughput = stats.success_count / duration_sec;

    std::cout << "========================================" << std::endl;
    std::cout << "Benchmark Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Duration: " << std::fixed << std::setprecision(2) 
              << duration_sec << "s" << std::endl;
    std::cout << std::endl;

    std::cout << "Requests:" << std::endl;
    std::cout << "  Total: " << total_completed << std::endl;
    std::cout << "  Success: " << stats.success_count << std::endl;
    std::cout << "  Errors: " << stats.error_count << std::endl;
    std::cout << "  Success Rate: " << std::fixed << std::setprecision(1) 
              << success_rate << "%" << std::endl;
    std::cout << std::endl;

    if (!stats.latencies.empty()) {
        std::cout << "Latency Distribution:" << std::endl;
        std::cout << "  p50  = " << std::fixed << std::setprecision(0) 
                  << percentile(stats.latencies, 0.50) << " ms" << std::endl;
        std::cout << "  p90  = " << percentile(stats.latencies, 0.90) << " ms" << std::endl;
        std::cout << "  p95  = " << percentile(stats.latencies, 0.95) << " ms" << std::endl;
        std::cout << "  p99  = " << percentile(stats.latencies, 0.99) << " ms" << std::endl;
        std::cout << "  p99.9 = " << percentile(stats.latencies, 0.999) << " ms" << std::endl;
        std::cout << std::endl;
    }

    std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
              << throughput << " req/s" << std::endl;
    std::cout << "========================================" << std::endl;

    // CSV output
    std::cout << std::endl;
    std::cout << "CSV Format:" << std::endl;
    std::cout << "url,concurrency,requests,duration_sec,success,errors,p50,p90,p95,p99,throughput" << std::endl;
    std::cout << url << ","
              << concurrency << ","
              << total_requests << ","
              << duration_sec << ","
              << stats.success_count << ","
              << stats.error_count << ","
              << percentile(stats.latencies, 0.50) << ","
              << percentile(stats.latencies, 0.90) << ","
              << percentile(stats.latencies, 0.95) << ","
              << percentile(stats.latencies, 0.99) << ","
              << throughput << std::endl;

    return 0;
}
