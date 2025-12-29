#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>


#include "httplib.h"
#include "json.hpp"
#include "sqlite3.h" 


using json = nlohmann::ordered_json;

#include "task.h"
#include "database.h"

using namespace httplib;


// Rate Limiting 
std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> rate_limit_map;
const int MAX_REQUESTS = 5; 
const int TIME_WINDOW = 60; 

// Authorization verification
bool api_gateway_check(const Request& req, Response& res) {
    if (!req.has_header("Authorization") || req.get_header_value("Authorization") != "gerestal") {
        res.status = 401;
        res.set_content("{\"error\":\"Invalid Token\"}", "application/json");
        return false;
    }

    std::string ip = req.remote_addr;
    auto now = std::chrono::steady_clock::now();

    auto& timestamps = rate_limit_map[ip];
    timestamps.erase(std::remove_if(timestamps.begin(), timestamps.end(),
        [&](auto t) { return std::chrono::duration_cast<std::chrono::seconds>(now - t).count() > TIME_WINDOW; }),
        timestamps.end());

    if (timestamps.size() >= MAX_REQUESTS) {
        res.status = 429;
        res.set_content("{\"error\":\"Too Many Requests. Wait a minute.\"}", "application/json");
        return false;
    }

    timestamps.push_back(now);
    return true;
}

// Logging
void log_request(const Request& req, int status) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    
    std::string time_str = std::ctime(&now_time);
    if (!time_str.empty()) time_str.pop_back();

    std::cout << "[" << time_str << "] "
        << req.method << " " << req.path
        << " -> " << status << std::endl;
}


int main() {

    system("chcp 65001");
    std::setlocale(LC_ALL, ".UTF8");

    Server svr;
    Database db("todo.db"); 

    // GET /tasks
    svr.Get("/tasks", [&](const Request& req, Response& res) {
       
        auto tasks = db.get_all_tasks();
        res.set_content(json(tasks).dump(), "application/json");

        res.set_header("Cache-Control", "public, max-age=30");
        res.set_header("ETag", std::to_string(std::hash<std::string>{}(json(tasks).dump())));

        log_request(req, 200);
        });

    // GET /tasks/{id}
    svr.Get("/tasks/:id", [&](const Request& req, Response& res) {
       
        int id = std::stoi(req.path_params.at("id"));
        Task t;
        if (db.get_task(id, t)) {
            res.set_content(json(t).dump(), "application/json");

            res.set_header("Cache-Control", "public, max-age=30");
            res.set_header("ETag", std::to_string(std::hash<std::string>{}(json(t).dump())));

            log_request(req, 200);
        }
        else {
            res.status = 404;
            res.set_content("{\"error\":\"Not Found\"}", "application/json");
            log_request(req, 404);
        }
        });

    // POST /tasks
    svr.Post("/tasks", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;
        try {
            auto j = json::parse(req.body);
            std::string title = j.value("title", "No Title");
            std::string desc = j.value("description", "");
            std::string status = j.value("status", "todo");

            int new_id = db.create_task(title, desc, status);

            json response_body = {
                {"id", new_id}, {"title", title}, {"description", desc}, {"status", status}
            };

            res.status = 201;
            res.set_content(response_body.dump(), "application/json");
            log_request(req, 201);
        }
        catch (...) {
            res.status = 400;
            log_request(req, 400);
        }
        });

    // PUT /tasks/{id}
    svr.Put("/tasks/:id", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;
        int id = std::stoi(req.path_params.at("id"));
        try {
            auto j = json::parse(req.body);
            if (db.update_task(id, j["title"], j["description"], j["status"])) {
                res.status = 200;
                Task t; db.get_task(id, t);
                res.set_content(json(t).dump(), "application/json");
                log_request(req, 200);
            }
            else {
                res.status = 404;
                log_request(req, 404);
            }
        }
        catch (...) {
            res.status = 400;
            log_request(req, 400);
        }
        });

    // PATCH /tasks/{id} 
    svr.Patch("/tasks/:id", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;
        try {
            int id = std::stoi(req.path_params.at("id"));
            auto j = json::parse(req.body);
            
            if (j.contains("status")) {
                std::string new_status = j["status"];

                if (db.update_task_status(id, new_status)) {
                    res.status = 200;

                    Task t;
                    db.get_task(id, t);
                    res.set_content(json(t).dump(), "application/json");

                    log_request(req, 200);
                }
                else {
                    res.status = 404;
                    log_request(req, 404);
                }
            }
            else {
                res.status = 400; 
                res.set_content("{\"error\":\"Missing status field\"}", "application/json");
            }
        }
        catch (...) {
            res.status = 400;
            log_request(req, 400);
        }
        });

    // DELETE /tasks/{id}
    svr.Delete("/tasks/:id", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;
        int id = std::stoi(req.path_params.at("id"));
        if (db.delete_task(id)) {
            res.status = 200;
            res.set_content("{\"message\":\"Deleted\"}", "application/json");
            log_request(req, 200);
        }
        else {
            res.status = 404;
            log_request(req, 404);
        }
        });

    svr.Get("/cache/stats", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;

        json stats = {
            {"cache_size", db.get_cache_size()}
        };

        res.set_content(stats.dump(), "application/json");
        log_request(req, 200);
        });

    svr.Delete("/cache", [&](const Request& req, Response& res) {
        if (!api_gateway_check(req, res)) return;

        db.clear_cache();
        res.status = 200;
        res.set_content("{\"message\":\"Cache cleared\"}", "application/json");
        log_request(req, 200);
        });

    std::cout << "Server started at http://localhost:8080 with SQLite backend" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}