#include "runtime/Environment.h"

#include <stdexcept>

Environment::Environment(std::shared_ptr<Environment> enclosing) : enclosing(std::move(enclosing)) {}

void Environment::define(const std::string& name, const Value& value) {
    values[name] = value;
}

void Environment::assign(const std::string& name, const Value& value) {
    auto found = values.find(name);
    if (found != values.end()) {
        found->second = value;
        return;
    }

    if (enclosing) {
        enclosing->assign(name, value);
        return;
    }

    throw std::runtime_error("Runtime Error: undefined variable '" + name + "'.");
}

Value Environment::get(const std::string& name) const {
    auto found = values.find(name);
    if (found != values.end()) {
        return found->second;
    }

    if (enclosing) {
        return enclosing->get(name);
    }

    throw std::runtime_error("Runtime Error: undefined variable '" + name + "'.");
}
