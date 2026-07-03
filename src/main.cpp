#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "interpreter/Interpreter.h"
#include "lexer/Lexer.h"
#include "lexer/Token.h"
#include "parser/Parser.h"

static std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool endsWith(const std::string& value, const std::string& suffix) {
    if (suffix.length() > value.length()) {
        return false;
    }

    return value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static bool isVionFilePath(const std::string& value) {
    return endsWith(value, ".vion");
}

static std::vector<Token> tokenize(const std::string& source) {
    Lexer lexer(source);
    return lexer.scanTokens();
}

static Program parseProgram(const std::string& source) {
    std::vector<Token> tokens = tokenize(source);
    Parser parser(std::move(tokens));
    return parser.parse();
}

static void printHelp() {
    std::cout << "Vion Language CLI\n";
    std::cout << "Usage:\n";
    std::cout << "  vion help                  Show help\n";
    std::cout << "  vion version               Show version\n";
    std::cout << "  vion <file.vion>           Run Vion program\n";
    std::cout << "  vion tokens <file.vion>    Print lexer tokens\n";
    std::cout << "  vion ast <file.vion>       Print parsed AST\n";
    std::cout << "  vion run <file.vion>       Run Vion program\n";
    std::cout << "  vion build <file.vion>     Build command placeholder\n";
}

static void printTokens(const std::string& source) {
    std::vector<Token> tokens = tokenize(source);

    std::cout << "Tokens:\n";
    std::cout << "------------------------\n";

    for (const Token& token : tokens) {
        std::cout
            << tokenTypeToString(token.type)
            << " | "
            << token.lexeme
            << " | line "
            << token.line
            << ", column "
            << token.column
            << "\n";
    }
}

static void printAst(const std::string& source) {
    Program program = parseProgram(source);

    std::cout << "AST:\n";
    std::cout << "------------------------\n";
    std::cout << program.toString();
}

static void runProgram(const std::string& source) {
    Program program = parseProgram(source);
    Interpreter interpreter;
    interpreter.interpret(program);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return 1;
    }

    std::string command = argv[1];

    try {
        if (command == "help" || command == "--help" || command == "-h") {
            printHelp();
            return 0;
        }

        if (command == "version" || command == "--version" || command == "-v") {
            std::cout << "Vion v0.2.0\n";
            return 0;
        }

        if (isVionFilePath(command)) {
            std::string source = readFile(command);
            runProgram(source);
            return 0;
        }

        if (argc < 3) {
            std::cerr << "Error: missing file path.\n";
            printHelp();
            return 1;
        }

        std::string filePath = argv[2];
        std::string source = readFile(filePath);

        if (command == "tokens") {
            printTokens(source);
            return 0;
        }

        if (command == "ast") {
            printAst(source);
            return 0;
        }

        if (command == "run") {
            runProgram(source);
            return 0;
        }

        if (command == "build") {
            std::cout << "Vion build is not implemented yet.\n";
            std::cout << "Current version is an interpreter. Use: vion run <file.vion>\n";
            return 0;
        }

        std::cerr << "Error: unknown command: " << command << "\n";
        printHelp();
        return 1;

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
