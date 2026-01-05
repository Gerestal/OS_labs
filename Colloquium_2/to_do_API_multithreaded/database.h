#pragma once
#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include "sqlite3.h"
#include "task.h"


// SQLite
class Database {
private:
    sqlite3* db;
    std::mutex db_mutex; 

    // Cache
    struct CacheEntry {
        std::vector<Task> tasks;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::map<std::string, CacheEntry> cache;
    mutable std::mutex cache_mutex;
    const int CACHE_TTL_SECONDS = 30;

public:
    Database(const std::string& db_name);
    ~Database();

   
    int create_task(const std::string& title, const std::string& desc, const std::string& status);
    std::vector<Task> get_all_tasks();
    bool get_task(int id, Task& t);
    bool update_task(int id, const std::string& title, const std::string& desc, const std::string& status);
    bool update_task_status(int id, const std::string& new_status);
    bool delete_task(int id);

    
    void clear_cache();
    size_t get_cache_size() const;

private:
    void create_table_if_not_exists();
    std::vector<Task> execute_query(const std::string& sql);
};

#endif