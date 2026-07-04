#pragma once

#include "parser/AST.h"
#include "vm/Chunk.h"
#include "vm/OpCode.h"
#include "vm/VM.h"

struct Local {
    std::string name;
    int depth;
    bool isCaptured = false; // true if captured by a closure
    bool isConst = false;    // true if defined with 'const'
};

// Describes how a closure captures a variable
struct CompilerUpvalue {
    uint8_t index;   // local index (if isLocal) or upvalue index in enclosing function
    bool isLocal;    // true = capture from enclosing locals, false = from enclosing upvalues
    bool isConst;    // true = captured variable is const
};

enum class FunctionType {
    TYPE_SCRIPT,
    TYPE_FUNCTION
};

struct LoopInfo {
    int startOffset;
    int scopeDepth;
    std::vector<int> breakJumps;
};

class Compiler {
public:
    Compiler(Compiler* enclosing, FunctionType type);
    
    // Compiles an AST program into a BytecodeFunction
    std::shared_ptr<BytecodeFunction> compile(const Program& program);
    std::shared_ptr<BytecodeFunction> endCompiler();

    // Public for class method compilation
    std::vector<Local> locals;

private:
    Compiler* enclosing;
    std::shared_ptr<BytecodeFunction> function;
    FunctionType type;

    int scopeDepth;
    std::vector<LoopInfo> loops;
    std::vector<CompilerUpvalue> upvalues; // upvalues captured by this function

    Chunk* currentChunk();

    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitConstant(Value value);
    
    int emitJump(uint8_t instruction);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    void beginScope();
    void endScope();
    void addLocal(const std::string& name, bool isConst = false);
    int resolveLocal(const std::string& name);
    int resolveUpvalue(const std::string& name);
    int addUpvalue(uint8_t index, bool isLocal, bool isConst);

    void compileStatement(const Stmt& stmt);
    void compileExpression(const Expr& expr);
    
    int currentLine = 0;
};
