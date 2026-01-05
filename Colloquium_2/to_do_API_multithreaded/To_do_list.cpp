#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>

#include "httplib.h"
#include "json.hpp"
#include "sqlite3.h" 

using json = nlohmann::ordered_json;
using namespace httplib;

// Rate Limiter
class RateLimiter {
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
};

RateLimiter rate_limiter;

// Authorization verification
bool check_auth(const Request& req) {
    return req.has_header("Authorization") &&
        req.get_header_value("Authorization") == "gerestal";
}

// Logging
void log_request(const Request& req, int status) {
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

#include "task.h"
#include "database.h"

int main() {
    system("chcp 65001");

    Server svr;
    Database db("todo.db");

    
    svr.Get("/health", [](const Request& req, Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
        });

    // GET/tasks
    svr.Get("/tasks", [&](const Request& req, Response& res) {
        try {
            auto tasks = db.get_all_tasks();
            res.set_content(json(tasks).dump(), "application/json");
            log_request(req, 200);
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal Server Error\"}", "application/json");
            log_request(req, 500);
            std::cerr << "Error in GET /tasks: " << e.what() << std::endl;
        }
        });

    // GET/tasks/{id}
    svr.Get("/tasks/:id", [&](const Request& req, Response& res) {
        try {
            int id = std::stoi(req.path_params.at("id"));
            Task t;
            if (db.get_task(id, t)) {
                res.set_content(json(t).dump(), "application/json");
                log_request(req, 200);
            }
            else {
                res.status = 404;
                res.set_content("{\"error\":\"Not Found\"}", "application/json");
                log_request(req, 404);
            }
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Bad Request\"}", "application/json");
            log_request(req, 400);
            std::cerr << "Error in GET /tasks/:id: " << e.what() << std::endl;
        }
        });

    // POST/tasks 
    svr.Post("/tasks", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }

        
        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            auto j = json::parse(req.body);
            std::string title = j.value("title", "No Title");
            std::string desc = j.value("description", "");
            std::string status = j.value("status", "todo");

            
            if (status != "todo" && status != "in_progress" && status != "done") {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid status\"}", "application/json");
                log_request(req, 400);
                return;
            }

            int new_id = db.create_task(title, desc, status);

            json response = {
                {"id", new_id},
                {"title", title},
                {"description", desc},
                {"status", status}
            };

            res.status = 201;
            res.set_content(response.dump(), "application/json");
            log_request(req, 201);
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Bad Request\"}", "application/json");
            log_request(req, 400);
            std::cerr << "Error in POST /tasks: " << e.what() << std::endl;
        }
        });

    // PUT/tasks/{id}
    svr.Put("/tasks/:id", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }

        
        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            int id = std::stoi(req.path_params.at("id"));
            auto j = json::parse(req.body);

            
            if (!j.contains("title") || !j.contains("description") || !j.contains("status")) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing required fields for PUT\"}", "application/json");
                log_request(req, 400);
                return;
            }

            std::string title = j["title"];
            std::string desc = j["description"];
            std::string status = j["status"];

            
            if (status != "todo" && status != "in_progress" && status != "done") {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid status\"}", "application/json");
                log_request(req, 400);
                return;
            }

            if (db.update_task(id, title, desc, status)) {
                
                Task updated_task;
                if (db.get_task(id, updated_task)) {
                    res.set_content(json(updated_task).dump(), "application/json");
                }
                else {
                    res.set_content("{\"message\":\"Task updated successfully\"}", "application/json");
                }
                log_request(req, 200);
            }
            else {
                res.status = 404;
                res.set_content("{\"error\":\"Task not found\"}", "application/json");
                log_request(req, 404);
            }
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Bad Request\"}", "application/json");
            log_request(req, 400);
            std::cerr << "Error in PUT /tasks/:id: " << e.what() << std::endl;
        }
        });

    // PATCH/tasks/{id} 
    svr.Patch("/tasks/:id", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }

        
        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            int id = std::stoi(req.path_params.at("id"));
            auto j = json::parse(req.body);

            
            if (!j.contains("status")) {
                res.status = 400;
                res.set_content("{\"error\":\"Missing status field for PATCH\"}", "application/json");
                log_request(req, 400);
                return;
            }

            std::string new_status = j["status"];

            
            if (new_status != "todo" && new_status != "in_progress" && new_status != "done") {
                res.status = 400;
                res.set_content("{\"error\":\"Invalid status\"}", "application/json");
                log_request(req, 400);
                return;
            }

            if (db.update_task_status(id, new_status)) {
                
                Task updated_task;
                if (db.get_task(id, updated_task)) {
                    res.set_content(json(updated_task).dump(), "application/json");
                }
                else {
                    res.set_content("{\"message\":\"Task status updated successfully\"}", "application/json");
                }
                log_request(req, 200);
            }
            else {
                res.status = 404;
                res.set_content("{\"error\":\"Task not found\"}", "application/json");
                log_request(req, 404);
            }
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Bad Request\"}", "application/json");
            log_request(req, 400);
            std::cerr << "Error in PATCH /tasks/:id: " << e.what() << std::endl;
        }
        });

    // DELETE/tasks/{id}
    svr.Delete("/tasks/:id", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }


        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            int id = std::stoi(req.path_params.at("id"));

            if (db.delete_task(id)) {
                res.status = 200;
                res.set_content("{\"message\":\"Task deleted successfully\"}", "application/json");
                log_request(req, 200);
            }
            else {
                res.status = 404;
                res.set_content("{\"error\":\"Task not found\"}", "application/json");
                log_request(req, 404);
            }
        }
        catch (const std::exception& e) {
            res.status = 400;
            res.set_content("{\"error\":\"Bad Request\"}", "application/json");
            log_request(req, 400);
            std::cerr << "Error in DELETE /tasks/:id: " << e.what() << std::endl;
        }
        });

    // GET/cache/stats 
    svr.Get("/cache/stats", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }

        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            size_t cache_size = db.get_cache_size();
            json stats = {
                {"cache_size", cache_size}
            };
            res.set_content(stats.dump(), "application/json");
            log_request(req, 200);
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal Server Error\"}", "application/json");
            log_request(req, 500);
            std::cerr << "Error in GET /cache/stats: " << e.what() << std::endl;
        }
        });

    // DELETE/cache
    svr.Delete("/cache", [&](const Request& req, Response& res) {
        
        if (!check_auth(req)) {
            res.status = 401;
            res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
            log_request(req, 401);
            return;
        }

        
        std::string ip = req.remote_addr;
        if (!rate_limiter.check(ip)) {
            res.status = 429;
            res.set_content("{\"error\":\"Too Many Requests\"}", "application/json");
            log_request(req, 429);
            return;
        }

        try {
            db.clear_cache();
            res.status = 200;
            res.set_content("{\"message\":\"Cache cleared successfully\"}", "application/json");
            log_request(req, 200);
        }
        catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\":\"Internal Server Error\"}", "application/json");
            log_request(req, 500);
            std::cerr << "Error in DELETE /cache: " << e.what() << std::endl;
        }
        });

   
    svr.set_error_handler([](const Request& req, Response& res) {
        if (res.status == 404) {
            json error = {
                {"error", "Endpoint not found"},
                {"path", req.path},
                {"method", req.method}
            };
            res.set_content(error.dump(), "application/json");
        }
        });

    std::cout << "Server started at http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}