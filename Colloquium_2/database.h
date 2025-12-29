#pragma once
#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include "sqlite3.h"
#include "task.h"


// SQLite
class Database {
private:
    sqlite3* db;

    //Cache
    struct CacheEntry {
        std::vector<Task> tasks;
        std::chrono::steady_clock::time_point timestamp;
    };

    std::map<std::string, CacheEntry> cache;
    std::mutex cache_mutex;
    const int CACHE_TTL_SECONDS = 30; 

   
    void invalidate_cache(const std::string& key);
    std::vector<Task> get_from_cache(const std::string& key);
    void save_to_cache(const std::string& key, const std::vector<Task>& tasks);
    bool is_cache_valid(const std::string& key);

public:
    Database(const std::string& db_name);
    ~Database();

    void create_table();
    int create_task(const std::string& title, const std::string& desc, const std::string& status);
    std::vector<Task> get_all_tasks();
    bool get_task(int id, Task& t);
    bool update_task(int id, const std::string& title, const std::string& desc, const std::string& status);
    bool update_task_status(int id, const std::string& new_status);
    bool delete_task(int id);

    
    void clear_cache();
    size_t get_cache_size() const;
};

#endif 
