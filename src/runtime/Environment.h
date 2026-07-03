#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "runtime/Value.h"

class Environment {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> enclosing);

    void define(const std::string& name, const Value& value);
    void assign(const std::string& name, const Value& value);
    Value get(const std::string& name) const;

private:
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing = nullptr;
};
