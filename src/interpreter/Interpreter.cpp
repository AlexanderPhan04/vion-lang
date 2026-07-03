#include "interpreter/Interpreter.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

// ── Control-flow signals ──────────────────────────────────────────────────────

namespace {

class ReturnSignal {
public:
    explicit ReturnSignal(Value value) : value(std::move(value)) {}
    Value value;
};

class BreakSignal {};
class ContinueSignal {};

// ── User-defined function ─────────────────────────────────────────────────────

class VionFunction final : public VionCallable {
public:
    VionFunction(const FunctionStmt& declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(std::move(closure)) {}

    int arity() const override {
        return static_cast<int>(declaration.parameters.size());
    }

    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override {
        auto callEnv = std::make_shared<Environment>(closure);

        for (std::size_t i = 0; i < declaration.parameters.size(); ++i) {
            callEnv->define(declaration.parameters[i], arguments[i]);
        }

        try {
            interpreter.executeBlock(*declaration.body, callEnv);
        } catch (const ReturnSignal& sig) {
            return sig.value;
        }

        return Value::nil();
    }

    std::string toString() const override {
        return "<fn " + declaration.name + ">";
    }

private:
    const FunctionStmt& declaration;
    std::shared_ptr<Environment> closure;
};

// ── Native function helper ────────────────────────────────────────────────────

class NativeFunction final : public VionCallable {
public:
    using Fn = std::function<Value(Interpreter&, const std::vector<Value>&)>;

    NativeFunction(std::string name, int arity, Fn fn)
        : name_(std::move(name)), arity_(arity), fn_(std::move(fn)) {}

    int arity() const override { return arity_; }

    Value call(Interpreter& interpreter, const std::vector<Value>& args) override {
        return fn_(interpreter, args);
    }

    std::string toString() const override { return "<native fn " + name_ + ">"; }

private:
    std::string name_;
    int arity_;
    Fn fn_;
};

// ── Error formatting ──────────────────────────────────────────────────────────

std::string argCountError(int expected, std::size_t received) {
    std::ostringstream out;
    out << "Runtime Error: expected " << expected
        << " argument(s) but got " << received << ".";
    return out.str();
}

} // namespace

// ── Interpreter ───────────────────────────────────────────────────────────────

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals) {
    registerBuiltins();
}

std::string Interpreter::locationOf(int line) const {
    if (line > 0) return " [line " + std::to_string(line) + "]";
    return "";
}

void Interpreter::registerBuiltins() {
    // len(val) — string length or array length
    globals->define("len", Value::function(std::make_shared<NativeFunction>(
        "len", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            const Value& v = args[0];
            if (v.type == ValueType::STRING)
                return Value::number(static_cast<double>(v.asString().size()));
            if (v.type == ValueType::ARRAY)
                return Value::number(static_cast<double>(v.asArray()->elements.size()));
            throw std::runtime_error("Runtime Error: len() expects a string or array.");
        }
    )));

    // push(arr, val) — append element to array, returns the array
    globals->define("push", Value::function(std::make_shared<NativeFunction>(
        "push", 2,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            if (args[0].type != ValueType::ARRAY)
                throw std::runtime_error("Runtime Error: push() expects an array as first argument.");
            args[0].asArray()->elements.push_back(args[1]);
            return args[0];
        }
    )));

    // pop(arr) — remove and return the last element
    globals->define("pop", Value::function(std::make_shared<NativeFunction>(
        "pop", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            if (args[0].type != ValueType::ARRAY)
                throw std::runtime_error("Runtime Error: pop() expects an array.");
            auto arr = args[0].asArray();
            if (arr->elements.empty())
                throw std::runtime_error("Runtime Error: pop() called on an empty array.");
            Value last = arr->elements.back();
            arr->elements.pop_back();
            return last;
        }
    )));

    // str(val) — convert any value to its string representation
    globals->define("str", Value::function(std::make_shared<NativeFunction>(
        "str", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::string(args[0].toString());
        }
    )));

    // num(val) — convert string to number
    globals->define("num", Value::function(std::make_shared<NativeFunction>(
        "num", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            if (args[0].type == ValueType::NUMBER) return args[0];
            if (args[0].type == ValueType::STRING) {
                try {
                    return Value::number(std::stod(args[0].asString()));
                } catch (...) {
                    throw std::runtime_error(
                        "Runtime Error: num() cannot convert \"" + args[0].asString() + "\" to a number.");
                }
            }
            throw std::runtime_error("Runtime Error: num() expects a string or number.");
        }
    )));

    // type(val) — return the type name as a string
    globals->define("type", Value::function(std::make_shared<NativeFunction>(
        "type", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::string(args[0].typeName());
        }
    )));

    // input(prompt) — read a line from stdin
    globals->define("input", Value::function(std::make_shared<NativeFunction>(
        "input", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            std::cout << args[0].toString();
            std::string line;
            if (!std::getline(std::cin, line)) return Value::nil();
            return Value::string(line);
        }
    )));

    // clock() — seconds since epoch (useful for timing)
    globals->define("clock", Value::function(std::make_shared<NativeFunction>(
        "clock", 0,
        [](Interpreter&, const std::vector<Value>&) -> Value {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            double secs = std::chrono::duration<double>(now).count();
            return Value::number(secs);
        }
    )));

    // floor(n)
    globals->define("floor", Value::function(std::make_shared<NativeFunction>(
        "floor", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::number(std::floor(args[0].asNumber()));
        }
    )));

    // ceil(n)
    globals->define("ceil", Value::function(std::make_shared<NativeFunction>(
        "ceil", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::number(std::ceil(args[0].asNumber()));
        }
    )));

    // sqrt(n)
    globals->define("sqrt", Value::function(std::make_shared<NativeFunction>(
        "sqrt", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            double v = args[0].asNumber();
            if (v < 0) throw std::runtime_error("Runtime Error: sqrt() of negative number.");
            return Value::number(std::sqrt(v));
        }
    )));

    // abs(n)
    globals->define("abs", Value::function(std::make_shared<NativeFunction>(
        "abs", 1,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::number(std::abs(args[0].asNumber()));
        }
    )));

    // max(a, b)
    globals->define("max", Value::function(std::make_shared<NativeFunction>(
        "max", 2,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::number(std::max(args[0].asNumber(), args[1].asNumber()));
        }
    )));

    // min(a, b)
    globals->define("min", Value::function(std::make_shared<NativeFunction>(
        "min", 2,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            return Value::number(std::min(args[0].asNumber(), args[1].asNumber()));
        }
    )));

    // array(size, fill) — create array with `size` elements initialized to `fill`
    globals->define("array", Value::function(std::make_shared<NativeFunction>(
        "array", 2,
        [](Interpreter&, const std::vector<Value>& args) -> Value {
            int size = static_cast<int>(args[0].asNumber());
            if (size < 0) throw std::runtime_error("Runtime Error: array() size cannot be negative.");
            auto arr = std::make_shared<VionArray>();
            arr->elements.assign(size, args[1]);
            return Value::array(arr);
        }
    )));
}

// ── Core interpreter ──────────────────────────────────────────────────────────

void Interpreter::interpret(const Program& program) {
    try {
        for (const auto& stmt : program.statements) {
            execute(*stmt);
        }
    } catch (const ReturnSignal&) {
        throw std::runtime_error("Runtime Error: return outside function.");
    } catch (const BreakSignal&) {
        throw std::runtime_error("Runtime Error: break outside loop.");
    } catch (const ContinueSignal&) {
        throw std::runtime_error("Runtime Error: continue outside loop.");
    }
}

void Interpreter::executeBlock(const BlockStmt& block, std::shared_ptr<Environment> blockEnv) {
    auto previous = environment;
    environment = std::move(blockEnv);

    try {
        for (const auto& stmt : block.statements) {
            execute(*stmt);
        }
    } catch (...) {
        environment = previous;
        throw;
    }

    environment = previous;
}

void Interpreter::execute(const Stmt& statement) {
    if (const auto* letStmt = dynamic_cast<const LetStmt*>(&statement)) {
        Value value = evaluate(*letStmt->value);
        environment->define(letStmt->name, value);
        return;
    }

    if (const auto* printStmt = dynamic_cast<const PrintStmt*>(&statement)) {
        Value value = evaluate(*printStmt->value);
        std::cout << value.toString() << "\n";
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ExpressionStmt*>(&statement)) {
        evaluate(*exprStmt->value);
        return;
    }

    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&statement)) {
        executeBlock(*blockStmt, std::make_shared<Environment>(environment));
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&statement)) {
        Value condition = evaluate(*ifStmt->condition);
        if (condition.isTruthy()) {
            executeBlock(*ifStmt->thenBranch, std::make_shared<Environment>(environment));
        } else if (ifStmt->elseBranch) {
            executeBlock(*ifStmt->elseBranch, std::make_shared<Environment>(environment));
        }
        return;
    }

    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&statement)) {
        while (evaluate(*whileStmt->condition).isTruthy()) {
            try {
                executeBlock(*whileStmt->body, std::make_shared<Environment>(environment));
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
        return;
    }

    if (const auto* forStmt = dynamic_cast<const ForStmt*>(&statement)) {
        Value iterableVal = evaluate(*forStmt->iterable);

        if (iterableVal.type == ValueType::ARRAY) {
            // Snapshot the array — iterate a copy of the element list
            auto elements = iterableVal.asArray()->elements;
            for (const Value& element : elements) {
                auto loopEnv = std::make_shared<Environment>(environment);
                loopEnv->define(forStmt->variable, element);
                try {
                    executeBlock(*forStmt->body, loopEnv);
                } catch (const BreakSignal&) {
                    return;
                } catch (const ContinueSignal&) {
                    continue;
                }
            }
        } else if (iterableVal.type == ValueType::STRING) {
            // Iterate over characters
            const std::string& s = iterableVal.asString();
            for (char ch : s) {
                auto loopEnv = std::make_shared<Environment>(environment);
                loopEnv->define(forStmt->variable, Value::string(std::string(1, ch)));
                try {
                    executeBlock(*forStmt->body, loopEnv);
                } catch (const BreakSignal&) {
                    return;
                } catch (const ContinueSignal&) {
                    continue;
                }
            }
        } else {
            throw std::runtime_error(
                "Runtime Error" + locationOf(forStmt->line) +
                ": 'for in' expects an array or string, got " + iterableVal.typeName() + "."
            );
        }
        return;
    }

    if (const auto* functionStmt = dynamic_cast<const FunctionStmt*>(&statement)) {
        environment->define(
            functionStmt->name,
            Value::function(std::make_shared<VionFunction>(*functionStmt, environment))
        );
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&statement)) {
        Value value = returnStmt->value ? evaluate(*returnStmt->value) : Value::nil();
        throw ReturnSignal(value);
    }

    if (dynamic_cast<const BreakStmt*>(&statement)) {
        throw BreakSignal{};
    }

    if (dynamic_cast<const ContinueStmt*>(&statement)) {
        throw ContinueSignal{};
    }

    throw std::runtime_error("Runtime Error: unknown statement type.");
}

// ── Expressions ───────────────────────────────────────────────────────────────

Value Interpreter::evaluate(const Expr& expression) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpr*>(&expression)) {
        return Value::number(numberExpr->value);
    }
    if (const auto* stringExpr = dynamic_cast<const StringExpr*>(&expression)) {
        return Value::string(stringExpr->value);
    }
    if (const auto* boolExpr = dynamic_cast<const BooleanExpr*>(&expression)) {
        return Value::boolean(boolExpr->value);
    }
    if (dynamic_cast<const NilExpr*>(&expression)) {
        return Value::nil();
    }
    if (const auto* identExpr = dynamic_cast<const IdentifierExpr*>(&expression)) {
        try {
            return environment->get(identExpr->name);
        } catch (const std::runtime_error&) {
            throw std::runtime_error(
                "Runtime Error" + locationOf(identExpr->line) +
                ": undefined variable '" + identExpr->name + "'."
            );
        }
    }
    if (const auto* assignExpr = dynamic_cast<const AssignmentExpr*>(&expression)) {
        Value value = evaluate(*assignExpr->value);
        try {
            environment->assign(assignExpr->name, value);
        } catch (const std::runtime_error&) {
            throw std::runtime_error(
                "Runtime Error" + locationOf(assignExpr->line) +
                ": undefined variable '" + assignExpr->name + "'."
            );
        }
        return value;
    }
    if (const auto* idxAssign = dynamic_cast<const IndexAssignExpr*>(&expression)) {
        Value obj = evaluate(*idxAssign->object);
        Value idx = evaluate(*idxAssign->index);
        Value val = evaluate(*idxAssign->value);

        if (obj.type != ValueType::ARRAY)
            throw std::runtime_error(
                "Runtime Error" + locationOf(idxAssign->line) + ": index assignment requires an array.");

        int i = static_cast<int>(idx.asNumber());
        auto arr = obj.asArray();
        if (i < 0) i = static_cast<int>(arr->elements.size()) + i;
        if (i < 0 || i >= static_cast<int>(arr->elements.size()))
            throw std::runtime_error(
                "Runtime Error" + locationOf(idxAssign->line) +
                ": array index " + std::to_string(i) + " out of bounds (size " +
                std::to_string(arr->elements.size()) + ").");

        arr->elements[i] = val;
        return val;
    }
    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expression)) {
        return evaluateUnary(*unaryExpr);
    }
    if (const auto* logicalExpr = dynamic_cast<const LogicalExpr*>(&expression)) {
        return evaluateLogical(*logicalExpr);
    }
    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expression)) {
        return evaluateBinary(*binaryExpr);
    }
    if (const auto* callExpr = dynamic_cast<const CallExpr*>(&expression)) {
        return evaluateCall(*callExpr);
    }
    if (const auto* indexExpr = dynamic_cast<const IndexExpr*>(&expression)) {
        return evaluateIndex(*indexExpr);
    }
    if (const auto* arrayExpr = dynamic_cast<const ArrayExpr*>(&expression)) {
        auto arr = std::make_shared<VionArray>();
        for (const auto& elem : arrayExpr->elements) {
            arr->elements.push_back(evaluate(*elem));
        }
        return Value::array(arr);
    }

    throw std::runtime_error("Runtime Error: unknown expression type.");
}

Value Interpreter::evaluateUnary(const UnaryExpr& expression) {
    Value right = evaluate(*expression.right);

    if (expression.op == "-") {
        if (right.type != ValueType::NUMBER)
            throw std::runtime_error(
                "Runtime Error" + locationOf(expression.line) + ": unary '-' expects a number.");
        return Value::number(-right.asNumber());
    }

    if (expression.op == "!") {
        return Value::boolean(!right.isTruthy());
    }

    throw std::runtime_error("Runtime Error: unknown unary operator '" + expression.op + "'.");
}

Value Interpreter::evaluateLogical(const LogicalExpr& expression) {
    Value left = evaluate(*expression.left);

    if (expression.op == "or") {
        if (left.isTruthy()) return left;
        return evaluate(*expression.right);
    }

    if (expression.op == "and") {
        if (!left.isTruthy()) return left;
        return evaluate(*expression.right);
    }

    throw std::runtime_error("Runtime Error: unknown logical operator '" + expression.op + "'.");
}

Value Interpreter::evaluateBinary(const BinaryExpr& expression) {
    Value left  = evaluate(*expression.left);
    Value right = evaluate(*expression.right);
    const std::string& op = expression.op;
    int line = expression.line;

    if (op == "+") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER)
            return Value::number(left.asNumber() + right.asNumber());
        if (left.type == ValueType::STRING || right.type == ValueType::STRING)
            return Value::string(left.toString() + right.toString());
        if (left.type == ValueType::ARRAY && right.type == ValueType::ARRAY) {
            auto result = std::make_shared<VionArray>();
            result->elements = left.asArray()->elements;
            for (const auto& e : right.asArray()->elements) result->elements.push_back(e);
            return Value::array(result);
        }
        throw std::runtime_error(
            "Runtime Error" + locationOf(line) + ": '+' requires numbers, strings, or arrays.");
    }

    if (op == "-") return Value::number(left.asNumber() - right.asNumber());
    if (op == "*") return Value::number(left.asNumber() * right.asNumber());

    if (op == "/") {
        double divisor = right.asNumber();
        if (std::abs(divisor) < 1e-12)
            throw std::runtime_error(
                "Runtime Error" + locationOf(line) + ": division by zero.");
        return Value::number(left.asNumber() / divisor);
    }

    if (op == "%") {
        double divisor = right.asNumber();
        if (std::abs(divisor) < 1e-12)
            throw std::runtime_error(
                "Runtime Error" + locationOf(line) + ": modulo by zero.");
        return Value::number(std::fmod(left.asNumber(), divisor));
    }

    if (op == ">")  return Value::boolean(left.asNumber() > right.asNumber());
    if (op == ">=") return Value::boolean(left.asNumber() >= right.asNumber());
    if (op == "<")  return Value::boolean(left.asNumber() < right.asNumber());
    if (op == "<=") return Value::boolean(left.asNumber() <= right.asNumber());
    if (op == "==") return Value::boolean(valuesEqual(left, right));
    if (op == "!=") return Value::boolean(!valuesEqual(left, right));

    throw std::runtime_error("Runtime Error: unknown binary operator '" + op + "'.");
}

Value Interpreter::evaluateCall(const CallExpr& expression) {
    Value callee = evaluate(*expression.callee);

    if (callee.type != ValueType::FUNCTION) {
        throw std::runtime_error(
            "Runtime Error" + locationOf(expression.line) + ": can only call functions.");
    }

    std::vector<Value> arguments;
    for (const auto& arg : expression.arguments) {
        arguments.push_back(evaluate(*arg));
    }

    auto fn = callee.asFunction();
    if (fn->arity() >= 0 && fn->arity() != static_cast<int>(arguments.size())) {
        throw std::runtime_error(argCountError(fn->arity(), arguments.size()));
    }

    return fn->call(*this, arguments);
}

Value Interpreter::evaluateIndex(const IndexExpr& expression) {
    Value obj = evaluate(*expression.object);
    Value idx = evaluate(*expression.index);
    int line = expression.line;

    if (obj.type == ValueType::ARRAY) {
        int i = static_cast<int>(idx.asNumber());
        auto arr = obj.asArray();
        if (i < 0) i = static_cast<int>(arr->elements.size()) + i;
        if (i < 0 || i >= static_cast<int>(arr->elements.size()))
            throw std::runtime_error(
                "Runtime Error" + locationOf(line) +
                ": array index " + std::to_string(i) + " out of bounds (size " +
                std::to_string(arr->elements.size()) + ").");
        return arr->elements[i];
    }

    if (obj.type == ValueType::STRING) {
        int i = static_cast<int>(idx.asNumber());
        const std::string& s = obj.asString();
        if (i < 0) i = static_cast<int>(s.size()) + i;
        if (i < 0 || i >= static_cast<int>(s.size()))
            throw std::runtime_error(
                "Runtime Error" + locationOf(line) +
                ": string index " + std::to_string(i) + " out of bounds.");
        return Value::string(std::string(1, s[i]));
    }

    throw std::runtime_error(
        "Runtime Error" + locationOf(line) +
        ": index operator requires array or string, got " + obj.typeName() + ".");
}

bool Interpreter::valuesEqual(const Value& left, const Value& right) const {
    if (left.type != right.type) return false;

    switch (left.type) {
        case ValueType::NUMBER:
            return std::abs(left.asNumber() - right.asNumber()) < 1e-12;
        case ValueType::STRING:
            return left.asString() == right.asString();
        case ValueType::BOOLEAN:
            return left.asBoolean() == right.asBoolean();
        case ValueType::FUNCTION:
            return left.asFunction() == right.asFunction();
        case ValueType::ARRAY:
            return left.asArray() == right.asArray(); // reference equality
        case ValueType::NIL:
            return true;
    }

    return false;
}
