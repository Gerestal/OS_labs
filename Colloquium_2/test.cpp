#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "database.h"
#include "task.h"

TEST_CASE("Database creates and reads tasks", "[database]") {

    Database db(":memory:");

    SECTION("Creating a task returns ID > 0") {
        int id = db.create_task("Test", "Description", "todo");
        REQUIRE(id > 0);
    }

    SECTION("Created task can be read") {
        int id = db.create_task("Readable task", "", "todo");
        Task task;
        bool found = db.get_task(id, task);

        REQUIRE(found == true);
        REQUIRE(task.title == "Readable task");
        REQUIRE(task.status == "todo");
    }
}

TEST_CASE("Database updates tasks", "[database]") {
    Database db(":memory:");

    SECTION("Full task update") {
        int id = db.create_task("Old", "Old description", "todo");
        bool updated = db.update_task(id, "New", "New description", "done");

        REQUIRE(updated == true);

        Task task;
        db.get_task(id, task);
        REQUIRE(task.title == "New");
        REQUIRE(task.status == "done");
    }

    SECTION("Update status only") {
        int id = db.create_task("Task", "Description", "todo");
        bool updated = db.update_task_status(id, "in_progress");

        REQUIRE(updated == true);

        Task task;
        db.get_task(id, task);
        REQUIRE(task.status == "in_progress");
        REQUIRE(task.title == "Task");
    }
}

TEST_CASE("Database deletes tasks", "[database]") {
    Database db(":memory:");

    int id = db.create_task("To delete", "", "todo");

    SECTION("Deleting an existing task") {
        bool deleted = db.delete_task(id);
        REQUIRE(deleted == true);

        Task task;
        bool found = db.get_task(id, task);
        REQUIRE(found == false);
    }

    SECTION("Deleting a non-existent task") {
        bool deleted = db.delete_task(9999);
        REQUIRE(deleted == false);
    }
}

TEST_CASE("Database cache", "[cache]") {
    Database db(":memory:");

    SECTION("Cache is empty initially") {
        REQUIRE(db.get_cache_size() == 0);
    }

    SECTION("Cache is populated after reading tasks") {
        db.create_task("Task 1", "", "todo");
        db.create_task("Task 2", "", "todo");

        auto tasks1 = db.get_all_tasks();
        auto tasks2 = db.get_all_tasks();

        REQUIRE(tasks1.size() == 2);
        REQUIRE(tasks2.size() == 2);
    }
}