#include "database.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

Database::Database(const std::string& db_name) {
   
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
        std::string err_msg = "Can't open database: ";
        err_msg += sqlite3_errmsg(db);
        throw std::runtime_error(err_msg);
    }

    
    const char* wal_sql = "PRAGMA journal_mode=WAL;";
    sqlite3_exec(db, wal_sql, nullptr, nullptr, nullptr);

    
    const char* timeout_sql = "PRAGMA busy_timeout = 5000;";
    sqlite3_exec(db, timeout_sql, nullptr, nullptr, nullptr);

   
    const char* fk_sql = "PRAGMA foreign_keys = ON;";
    sqlite3_exec(db, fk_sql, nullptr, nullptr, nullptr);

    create_table_if_not_exists();
}

Database::~Database() {
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_close(db);
}

void Database::create_table_if_not_exists() {
    std::lock_guard<std::mutex> lock(db_mutex);

    const char* sql =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "status TEXT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string error = "SQL error: ";
        error += errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }
}

std::vector<Task> Database::execute_query(const std::string& sql) {
    std::vector<Task> tasks;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Task t;
            t.id = sqlite3_column_int(stmt, 0);

            const char* title_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            t.title = title_ptr ? title_ptr : "";

            const char* desc_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            t.description = desc_ptr ? desc_ptr : "";

            const char* status_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            t.status = status_ptr ? status_ptr : "";

            tasks.push_back(t);
        }
    }

    if (stmt) sqlite3_finalize(stmt);
    return tasks;
}

int Database::create_task(const std::string& title, const std::string& desc, const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex);

    std::string sql = "INSERT INTO tasks (title, description, status) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return -1;
    }

    int id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    
    {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        cache.clear();
    }

    return id;
}

std::vector<Task> Database::get_all_tasks() {
    
    {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        auto it = cache.find("all_tasks");
        if (it != cache.end()) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.timestamp).count();

            if (elapsed < CACHE_TTL_SECONDS) {
                return it->second.tasks;
            }
        }
    }

    
    std::lock_guard<std::mutex> db_lock(db_mutex);
    std::vector<Task> tasks = execute_query(
        "SELECT id, title, description, status FROM tasks ORDER BY updated_at DESC;");

    
    {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        cache["all_tasks"] = { tasks, std::chrono::steady_clock::now() };
    }

    return tasks;
}

bool Database::get_task(int id, Task& t) {
    std::lock_guard<std::mutex> lock(db_mutex);

    std::string sql = "SELECT id, title, description, status FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool found = false;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        t.id = sqlite3_column_int(stmt, 0);

        const char* title_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.title = title_ptr ? title_ptr : "";

        const char* desc_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.description = desc_ptr ? desc_ptr : "";

        const char* status_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.status = status_ptr ? status_ptr : "";

        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::update_task(int id, const std::string& title, const std::string& desc, const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex);

    std::string sql = "UPDATE tasks SET title = ?, description = ?, status = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, desc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        cache.clear();
        return true;
    }

    return false;
}

bool Database::update_task_status(int id, const std::string& new_status) {
    std::lock_guard<std::mutex> lock(db_mutex);

    std::string sql = "UPDATE tasks SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, new_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        cache.clear();
        return true;
    }

    return false;
}

bool Database::delete_task(int id) {
    std::lock_guard<std::mutex> lock(db_mutex);

    std::string sql = "DELETE FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    
    if (result == SQLITE_DONE && sqlite3_changes(db) > 0) {
        std::lock_guard<std::mutex> cache_lock(cache_mutex);
        cache.clear();
        return true;
    }

    return false;
}

void Database::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache.clear();
}

size_t Database::get_cache_size() const {
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache.size();
}