#pragma once
#ifndef TASK_H
#define TASK_H

#include <string>
#include "json.hpp"

using json = nlohmann::ordered_json;

struct Task {
    int id;
    std::string title;
    std::string description;
    std::string status;


    Task() : id(0), title(""), description(""), status("") {}

    Task(int id, std::string title, std::string description, std::string status)
        : id(id), title(std::move(title)),
        description(std::move(description)),
        status(std::move(status)) {}

};

    inline void to_json(json& j, const Task& t) {
        j = json{ {"id", t.id}, {"title", t.title}, {"description", t.description}, {"status", t.status} };
    }

inline void from_json(const json& j, Task& t) {
    j.at("id").get_to(t.id);
    j.at("title").get_to(t.title);
    j.at("description").get_to(t.description);
    j.at("status").get_to(t.status);
}

#endif 