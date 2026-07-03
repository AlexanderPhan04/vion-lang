#pragma once

#include <sstream>
#include <stdexcept>
#include <memory>
#include <string>
#include <utility>
#include <variant>

class VionCallable;

enum class ValueType {
    NUMBER,
    STRING,
    BOOLEAN,
    FUNCTION,
    NIL
};

struct Value {
    ValueType type = ValueType::NIL;
    std::variant<double, std::string, bool, std::shared_ptr<VionCallable>> data = false;

    static Value number(double value) {
        Value result;
        result.type = ValueType::NUMBER;
        result.data = value;
        return result;
    }

    static Value string(std::string value) {
        Value result;
        result.type = ValueType::STRING;
        result.data = std::move(value);
        return result;
    }

    static Value boolean(bool value) {
        Value result;
        result.type = ValueType::BOOLEAN;
        result.data = value;
        return result;
    }

    static Value function(std::shared_ptr<VionCallable> value) {
        Value result;
        result.type = ValueType::FUNCTION;
        result.data = std::move(value);
        return result;
    }

    static Value nil() {
        return Value{};
    }

    double asNumber() const {
        if (type != ValueType::NUMBER) {
            throw std::runtime_error("Runtime Error: expected number.");
        }
        return std::get<double>(data);
    }

    const std::string& asString() const {
        if (type != ValueType::STRING) {
            throw std::runtime_error("Runtime Error: expected string.");
        }
        return std::get<std::string>(data);
    }

    bool asBoolean() const {
        if (type != ValueType::BOOLEAN) {
            throw std::runtime_error("Runtime Error: expected boolean.");
        }
        return std::get<bool>(data);
    }

    std::shared_ptr<VionCallable> asFunction() const {
        if (type != ValueType::FUNCTION) {
            throw std::runtime_error("Runtime Error: expected function.");
        }
        return std::get<std::shared_ptr<VionCallable>>(data);
    }

    bool isTruthy() const {
        switch (type) {
            case ValueType::NIL: return false;
            case ValueType::BOOLEAN: return std::get<bool>(data);
            case ValueType::NUMBER: return std::get<double>(data) != 0;
            case ValueType::STRING: return !std::get<std::string>(data).empty();
            case ValueType::FUNCTION: return true;
        }
        return false;
    }

    std::string toString() const {
        switch (type) {
            case ValueType::NUMBER: {
                std::ostringstream out;
                double value = std::get<double>(data);
                out << value;
                return out.str();
            }
            case ValueType::STRING:
                return std::get<std::string>(data);
            case ValueType::BOOLEAN:
                return std::get<bool>(data) ? "true" : "false";
            case ValueType::FUNCTION:
                return "<function>";
            case ValueType::NIL:
                return "nil";
        }
        return "nil";
    }
};
