#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct Expr {
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;
};

struct NumberExpr : Expr {
    double value;

    explicit NumberExpr(double value) : value(value) {}

    std::string toString() const override {
        std::ostringstream out;
        out << "Number(" << value << ")";
        return out.str();
    }
};

struct StringExpr : Expr {
    std::string value;

    explicit StringExpr(std::string value) : value(std::move(value)) {}

    std::string toString() const override {
        return "String(\"" + value + "\")";
    }
};

struct BooleanExpr : Expr {
    bool value;

    explicit BooleanExpr(bool value) : value(value) {}

    std::string toString() const override {
        return std::string("Boolean(") + (value ? "true" : "false") + ")";
    }
};

struct NilExpr : Expr {
    std::string toString() const override {
        return "Nil";
    }
};

struct IdentifierExpr : Expr {
    std::string name;

    explicit IdentifierExpr(std::string name) : name(std::move(name)) {}

    std::string toString() const override {
        return "Identifier(" + name + ")";
    }
};

struct AssignmentExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;

    AssignmentExpr(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string toString() const override {
        return "(assign " + name + " " + value->toString() + ")";
    }
};

struct UnaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> right;

    UnaryExpr(std::string op, std::unique_ptr<Expr> right)
        : op(std::move(op)), right(std::move(right)) {}

    std::string toString() const override {
        return "(" + op + " " + right->toString() + ")";
    }
};

struct LogicalExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;

    LogicalExpr(std::unique_ptr<Expr> left, std::string op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    std::string toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    std::string op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, std::string op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    std::string toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), arguments(std::move(arguments)) {}

    std::string toString() const override {
        std::ostringstream out;
        out << "Call " << callee->toString() << "(";
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            if (index > 0) out << ", ";
            out << arguments[index]->toString();
        }
        out << ")";
        return out.str();
    }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual std::string toString(int indent = 0) const = 0;
};

inline std::string indentText(int indent) {
    return std::string(indent, ' ');
}

struct LetStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    LetStmt(std::string name, std::unique_ptr<Expr> value)
        : name(std::move(name)), value(std::move(value)) {}

    std::string toString(int indent = 0) const override {
        return indentText(indent) + "Let " + name + " = " + value->toString();
    }
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit PrintStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

    std::string toString(int indent = 0) const override {
        return indentText(indent) + "Print " + value->toString();
    }
};

struct ExpressionStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit ExpressionStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

    std::string toString(int indent = 0) const override {
        return indentText(indent) + "Expr " + value->toString();
    }
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    std::string toString(int indent = 0) const override {
        std::ostringstream out;
        out << indentText(indent) << "Block\n";
        for (const auto& statement : statements) {
            out << statement->toString(indent + 2) << "\n";
        }
        return out.str();
    }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<BlockStmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    std::string toString(int indent = 0) const override {
        std::ostringstream out;
        out << indentText(indent) << "While " << condition->toString() << "\n";
        out << body->toString(indent + 2);
        return out.str();
    }
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBranch;
    std::unique_ptr<BlockStmt> elseBranch;

    IfStmt(
        std::unique_ptr<Expr> condition,
        std::unique_ptr<BlockStmt> thenBranch,
        std::unique_ptr<BlockStmt> elseBranch
    ) : condition(std::move(condition)),
        thenBranch(std::move(thenBranch)),
        elseBranch(std::move(elseBranch)) {}

    std::string toString(int indent = 0) const override {
        std::ostringstream out;
        out << indentText(indent) << "If " << condition->toString() << "\n";
        out << thenBranch->toString(indent + 2);
        if (elseBranch) {
            out << indentText(indent) << "Else\n";
            out << elseBranch->toString(indent + 2);
        }
        return out.str();
    }
};

struct FunctionStmt : Stmt {
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<BlockStmt> body;

    FunctionStmt(std::string name, std::vector<std::string> parameters, std::unique_ptr<BlockStmt> body)
        : name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}

    std::string toString(int indent = 0) const override {
        std::ostringstream out;
        out << indentText(indent) << "Fn " << name << "(";
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (index > 0) out << ", ";
            out << parameters[index];
        }
        out << ")\n";
        out << body->toString(indent + 2);
        return out.str();
    }
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit ReturnStmt(std::unique_ptr<Expr> value) : value(std::move(value)) {}

    std::string toString(int indent = 0) const override {
        if (!value) {
            return indentText(indent) + "Return";
        }
        return indentText(indent) + "Return " + value->toString();
    }
};

struct Program {
    std::vector<std::unique_ptr<Stmt>> statements;

    std::string toString() const {
        std::ostringstream out;
        out << "Program\n";
        for (const auto& statement : statements) {
            out << statement->toString(2) << "\n";
        }
        return out.str();
    }
};
