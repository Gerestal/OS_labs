#define _CRT_SECURE_NO_WARNINGS
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <iostream>
#include <ctime>
#include <sstream>


class MockRequest {
public:
    std::string method = "GET";
    std::string path = "/test";
    std::map<std::string, std::string> headers;

    bool has_header(const std::string& key) const {
        return headers.find(key) != headers.end();
    }

    std::string get_header_value(const std::string& key) const {
        auto it = headers.find(key);
        if (it != headers.end()) {
            return it->second;
        }
        return "";
    }
};


namespace {
    class TestRateLimiter {
    private:
        std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> requests;
        std::mutex mutex_;
        const int MAX_REQUESTS = 5;
        const int TIME_WINDOW = 60;

    public:
        bool check(const std::string& ip) {
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

        void clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            requests.clear();
        }

        size_t get_count(const std::string& ip) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = requests.find(ip);
            if (it == requests.end()) {
                return 0;
            }
            return it->second.size();
        }
    };
}


bool check_auth(const MockRequest& req) {
    return req.has_header("Authorization") &&
        req.get_header_value("Authorization") == "gerestal";
}


void log_request(const MockRequest& req, int status) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::string time_str = std::ctime(&now_time);
    if (!time_str.empty()) time_str.pop_back();

    std::cout << "[" << time_str << "] "
        << req.method << " " << req.path
        << " -> " << status
        << " [Thread: " << std::this_thread::get_id() << "]"
        << std::endl;
}


TEST_CASE("RateLimiter - Basic functionality", "[ratelimiter]") {
    TestRateLimiter limiter;
    limiter.clear();

    SECTION("First 5 requests pass") {
        for (int i = 0; i < 5; i++) {
            REQUIRE(limiter.check("192.168.1.1") == true);
        }
    }

    SECTION("6th request fails") {
        for (int i = 0; i < 5; i++) {
            limiter.check("192.168.1.1");
        }
        REQUIRE(limiter.check("192.168.1.1") == false);
    }

    SECTION("Different IPs have separate limits") {
        
        for (int i = 0; i < 5; i++) {
            limiter.check("192.168.1.1");
        }
        REQUIRE(limiter.check("192.168.1.1") == false); 

        
        REQUIRE(limiter.check("192.168.1.2") == true); 
    }

    SECTION("get_count returns correct values") {
        REQUIRE(limiter.get_count("192.168.1.100") == 0);

        limiter.check("192.168.1.101");
        limiter.check("192.168.1.101");
        limiter.check("192.168.1.101");

        REQUIRE(limiter.get_count("192.168.1.101") == 3);
    }
}


TEST_CASE("check_auth - Authorization checks", "[auth]") {
    SECTION("Valid token passes") {
        MockRequest req;
        req.headers["Authorization"] = "gerestal";
        REQUIRE(check_auth(req) == true);
    }

    SECTION("Missing authorization fails") {
        MockRequest req;
        REQUIRE(check_auth(req) == false);
    }

    SECTION("Wrong token fails") {
        MockRequest req;
        req.headers["Authorization"] = "wrong_token";
        REQUIRE(check_auth(req) == false);
    }

    SECTION("Case sensitive check") {
        MockRequest req1;
        req1.headers["Authorization"] = "Gerestal"; 
        REQUIRE(check_auth(req1) == false);

        MockRequest req2;
        req2.headers["Authorization"] = "GERESTAL"; 
        REQUIRE(check_auth(req2) == false);

        MockRequest req3;
        req3.headers["Authorization"] = "gerestal"; 
        REQUIRE(check_auth(req3) == true);
    }
}


TEST_CASE("log_request - Logging functionality", "[logging]") {
   
    SECTION("Function doesn't crash") {
        MockRequest req;
        req.method = "POST";
        req.path = "/api/data";

        
        REQUIRE_NOTHROW(log_request(req, 200));
    }

    SECTION("Different status codes") {
        MockRequest req;

        REQUIRE_NOTHROW(log_request(req, 200));
        REQUIRE_NOTHROW(log_request(req, 404));
        REQUIRE_NOTHROW(log_request(req, 500));
        REQUIRE_NOTHROW(log_request(req, 429));
    }
}

TEST_CASE("Integration - Combined functionality", "[integration]") {
    SECTION("Complete request flow") {
        TestRateLimiter limiter;
        limiter.clear();

        MockRequest req;
        req.method = "GET";
        req.path = "/api/test";
        req.headers["Authorization"] = "gerestal";

        
        REQUIRE(check_auth(req) == true);

        
        for (int i = 0; i < 5; i++) {
            REQUIRE(limiter.check("192.168.1.1") == true);
        }
        REQUIRE(limiter.check("192.168.1.1") == false);

        
        REQUIRE_NOTHROW(log_request(req, 200));
        REQUIRE_NOTHROW(log_request(req, 429));
    }
}