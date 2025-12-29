#include "database.h"
#include <iostream>
#include <stdexcept>

Database::Database(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }
    create_table();
}

Database::~Database() {
    sqlite3_close(db);
}

void Database::create_table() {
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

bool Database::is_cache_valid(const std::string& key) {
    auto it = cache.find(key);
    if (it == cache.end()) return false;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - it->second.timestamp).count();

    return elapsed < CACHE_TTL_SECONDS;
}

std::vector<Task> Database::get_from_cache(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (is_cache_valid(key)) {
        return cache[key].tasks;
    }
    return {};
}

void Database::save_to_cache(const std::string& key, const std::vector<Task>& tasks) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache[key] = { tasks, std::chrono::steady_clock::now() };
}

void Database::invalidate_cache(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache.erase(key);
}

void Database::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache.clear();
}

size_t Database::get_cache_size() const {
    return cache.size();
}

int Database::create_task(const std::string& title, const std::string& desc, const std::string& status) {
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

   
    invalidate_cache("all_tasks");

    return id;
}

std::vector<Task> Database::get_all_tasks() {
    
    std::vector<Task> cached_tasks = get_from_cache("all_tasks");
    if (!cached_tasks.empty()) {
        return cached_tasks;
    }

    
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

    
    save_to_cache("all_tasks", tasks);

    return tasks;
}

bool Database::get_task(int id, Task& t) {
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

bool Database::update_task(int id, const std::string& title, const std::string& desc, const std::string& status) {
    std::string sql = "UPDATE tasks SET title = ?, description = ?, status = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        invalidate_cache("all_tasks");
        return true;
    }

    return false;
}

bool Database::update_task_status(int id, const std::string& new_status) {
    std::string sql = "UPDATE tasks SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, new_status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        invalidate_cache("all_tasks");
        return true;
    }

    return false;
}

bool Database::delete_task(int id) {
    std::string sql = "DELETE FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

   
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        invalidate_cache("all_tasks");
        return true;
    }

    return false;
}