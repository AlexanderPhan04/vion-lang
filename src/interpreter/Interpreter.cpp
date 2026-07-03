#include "interpreter/Interpreter.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

class ReturnSignal {
public:
    explicit ReturnSignal(Value value) : value(std::move(value)) {}

    Value value;
};

class VionFunction final : public VionCallable {
public:
    VionFunction(const FunctionStmt& declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(std::move(closure)) {}

    int arity() const override {
        return static_cast<int>(declaration.parameters.size());
    }

    Value call(Interpreter& interpreter, const std::vector<Value>& arguments) override {
        auto callEnvironment = std::make_shared<Environment>(closure);

        for (std::size_t index = 0; index < declaration.parameters.size(); ++index) {
            callEnvironment->define(declaration.parameters[index], arguments[index]);
        }

        try {
            interpreter.executeBlock(*declaration.body, callEnvironment);
        } catch (const ReturnSignal& signal) {
            return signal.value;
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

std::string argumentCountError(int expected, std::size_t received) {
    std::ostringstream out;
    out << "Runtime Error: expected " << expected
        << " argument(s) but got " << received << ".";
    return out.str();
}

}

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals) {}

void Interpreter::interpret(const Program& program) {
    try {
        for (const auto& statement : program.statements) {
            execute(*statement);
        }
    } catch (const ReturnSignal&) {
        throw std::runtime_error("Runtime Error: return outside function.");
    }
}

void Interpreter::executeBlock(const BlockStmt& block, std::shared_ptr<Environment> blockEnvironment) {
    auto previous = environment;
    environment = std::move(blockEnvironment);

    try {
        for (const auto& statement : block.statements) {
            execute(*statement);
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

    if (const auto* expressionStmt = dynamic_cast<const ExpressionStmt*>(&statement)) {
        evaluate(*expressionStmt->value);
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
            executeBlock(*whileStmt->body, std::make_shared<Environment>(environment));
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

    throw std::runtime_error("Runtime Error: unknown statement type.");
}

Value Interpreter::evaluate(const Expr& expression) {
    if (const auto* numberExpr = dynamic_cast<const NumberExpr*>(&expression)) {
        return Value::number(numberExpr->value);
    }

    if (const auto* stringExpr = dynamic_cast<const StringExpr*>(&expression)) {
        return Value::string(stringExpr->value);
    }

    if (const auto* booleanExpr = dynamic_cast<const BooleanExpr*>(&expression)) {
        return Value::boolean(booleanExpr->value);
    }

    if (dynamic_cast<const NilExpr*>(&expression)) {
        return Value::nil();
    }

    if (const auto* identifierExpr = dynamic_cast<const IdentifierExpr*>(&expression)) {
        return environment->get(identifierExpr->name);
    }

    if (const auto* assignmentExpr = dynamic_cast<const AssignmentExpr*>(&expression)) {
        Value value = evaluate(*assignmentExpr->value);
        environment->assign(assignmentExpr->name, value);
        return value;
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

    throw std::runtime_error("Runtime Error: unknown expression type.");
}

Value Interpreter::evaluateUnary(const UnaryExpr& expression) {
    Value right = evaluate(*expression.right);

    if (expression.op == "-") {
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
    Value left = evaluate(*expression.left);
    Value right = evaluate(*expression.right);

    const std::string& op = expression.op;

    if (op == "+") {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) {
            return Value::number(left.asNumber() + right.asNumber());
        }
        if (left.type == ValueType::STRING || right.type == ValueType::STRING) {
            return Value::string(left.toString() + right.toString());
        }
        throw std::runtime_error("Runtime Error: operator '+' expects numbers or strings.");
    }

    if (op == "-") {
        return Value::number(left.asNumber() - right.asNumber());
    }

    if (op == "*") {
        return Value::number(left.asNumber() * right.asNumber());
    }

    if (op == "/") {
        double divisor = right.asNumber();
        if (std::abs(divisor) < 1e-12) {
            throw std::runtime_error("Runtime Error: division by zero.");
        }
        return Value::number(left.asNumber() / divisor);
    }

    if (op == ">") {
        return Value::boolean(left.asNumber() > right.asNumber());
    }

    if (op == ">=") {
        return Value::boolean(left.asNumber() >= right.asNumber());
    }

    if (op == "<") {
        return Value::boolean(left.asNumber() < right.asNumber());
    }

    if (op == "<=") {
        return Value::boolean(left.asNumber() <= right.asNumber());
    }

    if (op == "==") {
        return Value::boolean(valuesEqual(left, right));
    }

    if (op == "!=") {
        return Value::boolean(!valuesEqual(left, right));
    }

    throw std::runtime_error("Runtime Error: unknown binary operator '" + op + "'.");
}

Value Interpreter::evaluateCall(const CallExpr& expression) {
    Value callee = evaluate(*expression.callee);
    if (callee.type != ValueType::FUNCTION) {
        throw std::runtime_error("Runtime Error: can only call functions.");
    }

    std::vector<Value> arguments;
    for (const auto& argument : expression.arguments) {
        arguments.push_back(evaluate(*argument));
    }

    auto function = callee.asFunction();
    if (function->arity() != static_cast<int>(arguments.size())) {
        throw std::runtime_error(argumentCountError(function->arity(), arguments.size()));
    }

    return function->call(*this, arguments);
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
        case ValueType::NIL:
            return true;
    }

    return false;
}
