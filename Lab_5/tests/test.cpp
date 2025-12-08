#include "Header.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <fstream>

using namespace std;



SCENARIO("Работа с файлами сотрудников") {
    GIVEN("Файл с сотрудниками") {
        employee emp = { 1, "John", 40.5 };
        ofstream file("test.bin", ios::binary);
        file.write(reinterpret_cast<const char*>(&emp), sizeof(employee));
        file.close();

        WHEN("Ищем существующего сотрудника") {
            employee result = FindEmployeeInFile(1, "test.bin");

            THEN("Должен быть найден") {
                REQUIRE(result.num == 1);
                REQUIRE(string(result.name) == "John");
                REQUIRE(result.hours == 40.5);
            }
        }

        WHEN("Модифицируем сотрудника") {
            employee newEmp = { 1, "Updated", 100.0 };
            bool success = ModifyEmployeeInFile(1, newEmp, "test.bin");

            THEN("Модификация должна быть успешной") {
                REQUIRE(success);

                AND_THEN("Данные должны измениться") {
                    employee result = FindEmployeeInFile(1, "test.bin");
                    REQUIRE(string(result.name) == "Updated");
                }
            }
        }

        remove("test.bin");
    }
}

