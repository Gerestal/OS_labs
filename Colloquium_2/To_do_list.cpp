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

struct Task {
    int id;
    std::string title;
    std::string description;
    std::string status;
};

void to_json(json& j, const Task& t) {
    j = json{ {"id", t.id}, {"title", t.title}, {"description", t.description}, {"status", t.status} };
}


// SQLite
class Database {
private:
    sqlite3* db;

public:
    Database(const std::string& db_name) {
        if (sqlite3_open(db_name.c_str(), &db)) {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            exit(1);
        }
        create_table();
    }

    ~Database() {
        sqlite3_close(db);
    }

    void create_table() {
        const char* sql = "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "description TEXT,"
            "status TEXT NOT NULL);";
        char* errMsg = 0;
        if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }

    int create_task(const std::string& title, const std::string& desc, const std::string& status) {
        std::string sql = "INSERT INTO tasks (title, description, status) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        }

        int id = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        return id;
    }

    std::vector<Task> get_all_tasks() {
        std::vector<Task> tasks;
        std::string sql = "SELECT id, title, description, status FROM tasks;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Task t;
                t.id = sqlite3_column_int(stmt, 0);
                t.title = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
                t.description = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                t.status = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
                tasks.push_back(t);
            }
        }
        sqlite3_finalize(stmt);
        return tasks;
    }

    bool get_task(int id, Task& t) {
        std::string sql = "SELECT id, title, description, status FROM tasks WHERE id = ?;";
        sqlite3_stmt* stmt;
        bool found = false;

        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            t.id = sqlite3_column_int(stmt, 0);
            t.title = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            t.description = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            t.status = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            found = true;
        }
        sqlite3_finalize(stmt);
        return found;
    }

    bool update_task(int id, const std::string& title, const std::string& desc, const std::string& status) {
        std::string sql = "UPDATE tasks SET title = ?, description = ?, status = ? WHERE id = ?;";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, id);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (result == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    }

    bool update_task_status(int id, const std::string& new_status) {
        std::string sql = "UPDATE tasks SET status = ? WHERE id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, new_status.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, id);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (result == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    }

    bool delete_task(int id) {
        std::string sql = "DELETE FROM tasks WHERE id = ?;";
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, id);

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        return (result == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    }
};


int main() {

    system("chcp 65001");
    std::setlocale(LC_ALL, ".UTF8");

    Server svr;
    Database db("todo.db"); 

    // GET /tasks
    svr.Get("/tasks", [&](const Request& req, Response& res) {
       
        auto tasks = db.get_all_tasks();
        res.set_content(json(tasks).dump(), "application/json");
        log_request(req, 200);
        });

    // GET /tasks/{id}
    svr.Get("/tasks/:id", [&](const Request& req, Response& res) {
       
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

    std::cout << "Server started at http://localhost:8080 with SQLite backend" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}