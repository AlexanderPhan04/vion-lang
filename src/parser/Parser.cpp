#include "parser/Parser.h"

#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

Program Parser::parse() {
    Program program;

    while (!isAtEnd()) {
        program.statements.push_back(declaration());
    }

    return program;
}

std::unique_ptr<Stmt> Parser::declaration() {
    if (match(TokenType::FN)) return functionDeclaration();

    return statement();
}

std::unique_ptr<Stmt> Parser::functionDeclaration() {
    const Token& name = consume(TokenType::IDENTIFIER, "expected function name after 'fn'.");
    consume(TokenType::LEFT_PAREN, "expected '(' after function name.");

    std::vector<std::string> parameters;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            parameters.push_back(consume(TokenType::IDENTIFIER, "expected parameter name.").lexeme);
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_PAREN, "expected ')' after function parameters.");
    consume(TokenType::LEFT_BRACE, "expected '{' before function body.");

    return std::make_unique<FunctionStmt>(name.lexeme, std::move(parameters), blockStatement());
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match(TokenType::LET)) return letStatement();
    if (match(TokenType::PRINT)) return printStatement();
    if (match(TokenType::IF)) return ifStatement();
    if (match(TokenType::WHILE)) return whileStatement();
    if (match(TokenType::RETURN)) return returnStatement();
    if (match(TokenType::LEFT_BRACE)) return blockStatement();

    return expressionStatement();
}

std::unique_ptr<Stmt> Parser::letStatement() {
    const Token& name = consume(TokenType::IDENTIFIER, "expected variable name after 'let'.");
    consume(TokenType::EQUAL, "expected '=' after variable name.");
    auto value = expression();
    return std::make_unique<LetStmt>(name.lexeme, std::move(value));
}

std::unique_ptr<Stmt> Parser::printStatement() {
    auto value = expression();
    return std::make_unique<PrintStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::ifStatement() {
    auto condition = expression();

    consume(TokenType::LEFT_BRACE, "expected '{' after if condition.");
    auto thenBranch = blockStatement();

    std::unique_ptr<BlockStmt> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        consume(TokenType::LEFT_BRACE, "expected '{' after else.");
        elseBranch = blockStatement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Stmt> Parser::whileStatement() {
    auto condition = expression();

    consume(TokenType::LEFT_BRACE, "expected '{' after while condition.");
    auto body = blockStatement();

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::returnStatement() {
    if (check(TokenType::RIGHT_BRACE) || check(TokenType::END_OF_FILE)) {
        return std::make_unique<ReturnStmt>(nullptr);
    }

    return std::make_unique<ReturnStmt>(expression());
}

std::unique_ptr<Stmt> Parser::expressionStatement() {
    return std::make_unique<ExpressionStmt>(expression());
}

std::unique_ptr<BlockStmt> Parser::blockStatement() {
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }

    consume(TokenType::RIGHT_BRACE, "expected '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    auto expr = logicalOr();

    if (match(TokenType::EQUAL)) {
        const Token& equalsToken = previous();
        auto value = assignment();

        if (const auto* identifier = dynamic_cast<const IdentifierExpr*>(expr.get())) {
            return std::make_unique<AssignmentExpr>(identifier->name, std::move(value));
        }

        errorAt(equalsToken, "invalid assignment target.");
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();

    while (match(TokenType::OR)) {
        std::string op = previous().lexeme;
        auto right = logicalAnd();
        expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();

    while (match(TokenType::AND)) {
        std::string op = previous().lexeme;
        auto right = equality();
        expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();

    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        std::string op = previous().lexeme;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();

    while (
        match(TokenType::GREATER) ||
        match(TokenType::GREATER_EQUAL) ||
        match(TokenType::LESS) ||
        match(TokenType::LESS_EQUAL)
    ) {
        std::string op = previous().lexeme;
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();

    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        std::string op = previous().lexeme;
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();

    while (match(TokenType::STAR) || match(TokenType::SLASH)) {
        std::string op = previous().lexeme;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match(TokenType::BANG) || match(TokenType::MINUS)) {
        std::string op = previous().lexeme;
        auto right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }

    return call();
}

std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();

    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall(std::move(expr));
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expr> Parser::finishCall(std::unique_ptr<Expr> callee) {
    std::vector<std::unique_ptr<Expr>> arguments;

    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            arguments.push_back(expression());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_PAREN, "expected ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(arguments));
}

std::unique_ptr<Expr> Parser::primary() {
    if (match(TokenType::NUMBER)) {
        return std::make_unique<NumberExpr>(std::stod(previous().lexeme));
    }

    if (match(TokenType::STRING)) {
        return std::make_unique<StringExpr>(previous().lexeme);
    }

    if (match(TokenType::TRUE)) {
        return std::make_unique<BooleanExpr>(true);
    }

    if (match(TokenType::FALSE)) {
        return std::make_unique<BooleanExpr>(false);
    }

    if (match(TokenType::NIL)) {
        return std::make_unique<NilExpr>();
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<IdentifierExpr>(previous().lexeme);
    }

    if (match(TokenType::LEFT_PAREN)) {
        auto expr = expression();
        consume(TokenType::RIGHT_PAREN, "expected ')' after expression.");
        return expr;
    }

    errorAtCurrent("expected expression.");
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    errorAtCurrent(message);
}

void Parser::errorAt(const Token& token, const std::string& message) const {
    throw std::runtime_error(
        "Parser Error at line " + std::to_string(token.line) +
        ", column " + std::to_string(token.column) +
        ": " + message
    );
}

void Parser::errorAtCurrent(const std::string& message) const {
    errorAt(peek(), message);
}
