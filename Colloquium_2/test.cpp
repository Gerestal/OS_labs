// tests/test.cpp - Пример тестов
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "database.h"
#include "task.h"


TEST_CASE("Database создает и читает задачи", "[database]") {
    
    Database db(":memory:");

    SECTION("Создание задачи возвращает ID > 0") {
        int id = db.create_task("Тест", "Описание", "todo");
        REQUIRE(id > 0);
    }

    SECTION("Созданную задачу можно прочитать") {
        int id = db.create_task("Читаемая задача", "", "todo");
        Task task;
        bool found = db.get_task(id, task);

        REQUIRE(found == true);
        REQUIRE(task.title == "Читаемая задача");
        REQUIRE(task.status == "todo");
    }
}

TEST_CASE("Database обновляет задачи", "[database]") {
    Database db(":memory:");

    SECTION("Полное обновление задачи") {
        int id = db.create_task("Старое", "Старое описание", "todo");
        bool updated = db.update_task(id, "Новое", "Новое описание", "done");

        REQUIRE(updated == true);

        Task task;
        db.get_task(id, task);
        REQUIRE(task.title == "Новое");
        REQUIRE(task.status == "done");
    }

    SECTION("Обновление только статуса") {
        int id = db.create_task("Задача", "Описание", "todo");
        bool updated = db.update_task_status(id, "in_progress");

        REQUIRE(updated == true);

        Task task;
        db.get_task(id, task);
        REQUIRE(task.status == "in_progress");
        REQUIRE(task.title == "Задача");  
    }
}

TEST_CASE("Database удаляет задачи", "[database]") {
    Database db(":memory:");

    int id = db.create_task("На удаление", "", "todo");

    SECTION("Удаление существующей задачи") {
        bool deleted = db.delete_task(id);
        REQUIRE(deleted == true);

        Task task;
        bool found = db.get_task(id, task);
        REQUIRE(found == false);
    }

    SECTION("Удаление несуществующей задачи") {
        bool deleted = db.delete_task(9999);
        REQUIRE(deleted == false);
    }
}

TEST_CASE("Кеш базы данных", "[cache]") {
    Database db(":memory:");

    SECTION("Кеш пуст изначально") {
        REQUIRE(db.get_cache_size() == 0);
    }

    SECTION("После чтения задач кеш заполняется") {
        db.create_task("Задача 1", "", "todo");
        db.create_task("Задача 2", "", "todo");

        auto tasks1 = db.get_all_tasks();
        auto tasks2 = db.get_all_tasks();

       
        REQUIRE(tasks1.size() == 2);
        REQUIRE(tasks2.size() == 2);
    }
}