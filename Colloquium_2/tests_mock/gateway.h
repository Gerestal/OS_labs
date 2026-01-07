#ifndef GATEWAY_H
#define GATEWAY_H

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <iostream>
#include <ctime>

// Объявляем зависимости от httplib
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::ordered_json;

class RateLimiter {
private:
    std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests;
    std::mutex mutex_;
    const int MAX_REQUESTS = 5;
    const int TIME_WINDOW = 60;

public:
    bool check(const std::string& ip);
    void clear(); // для тестов
    size_t get_count(const std::string& ip); // для тестов
};

// Объявляем экземпляр RateLimiter
extern RateLimiter rate_limiter;

// Authorization verification
bool check_auth(const httplib::Request& req);

// Logging
void log_request(const httplib::Request& req, int status);

#endif // GATEWAY_H