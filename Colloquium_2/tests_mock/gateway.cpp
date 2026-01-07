#define _CRT_SECURE_NO_WARNINGS
#include "gateway.h"
#include <iomanip> 


RateLimiter rate_limiter;


bool RateLimiter::check(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    auto& timestamps = requests[ip];

    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
            [&](auto t) {
                return std::chrono::duration_cast<std::chrono::seconds>(now - t).count() > TIME_WINDOW;
            }),
        timestamps.end());

    if (timestamps.size() >= MAX_REQUESTS) {
        return false;
    }

    timestamps.push_back(now);
    return true;
}


void RateLimiter::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    requests.clear();
}


size_t RateLimiter::get_count(const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = requests.find(ip);
    if (it == requests.end()) {
        return 0;
    }
    return it->second.size();
}

// Authorization verification
bool check_auth(const httplib::Request& req) {
    return req.has_header("Authorization") &&
        req.get_header_value("Authorization") == "gerestal";
}

// Logging
void log_request(const httplib::Request& req, int status) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm tm_info;

#ifdef _WIN32
    localtime_s(&tm_info, &now_time);
#else
    localtime_r(&now_time, &tm_info);
#endif

    std::cout << "[";
    std::cout << std::put_time(&tm_info, "%Y-%m-%d %H:%M:%S");
    std::cout << "] "
        << req.method << " " << req.path
        << " -> " << status
        << " [Thread: " << std::this_thread::get_id() << "]"
        << std::endl;
}