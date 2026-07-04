#pragma once

#include <vector>
#include "vm/Chunk.h"
#include "runtime/Value.h"

#include <unordered_map>
#include <string>

enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

#include <memory>

struct BytecodeFunction : public GCObject {
    int arity = 0;
    int requiredArity = 0;  // params without defaults
    int upvalueCount = 0;   // number of upvalues this function captures
    std::shared_ptr<Chunk> chunk;
    std::string name;
    
    BytecodeFunction() {
        chunk = std::make_shared<Chunk>();
    }

    void trace(std::vector<std::shared_ptr<GCObject>>& children) const override {}
    void breakCycles() override { chunk.reset(); }
};

// Upvalue: a heap-allocated box for captured variables.
// Shared between the enclosing scope and any closures that capture it.
struct ObjUpvalue {
    Value value;
    ObjUpvalue() : value(Value::nil()) {}
    explicit ObjUpvalue(Value v) : value(std::move(v)) {}
};

struct CallFrame {
    std::shared_ptr<BytecodeFunction> function;
    uint8_t* ip;
    int slots_base;
    std::vector<std::shared_ptr<ObjUpvalue>> upvalues; // captured variables for this closure
};

struct TryHandler {
    int frameIndex;
    int stackSize;
    uint8_t* catchIp;
};

class VM {
public:
    VM();
    ~VM();

    InterpretResult interpret(std::shared_ptr<BytecodeFunction> function, const std::string& scriptPath = "");
    void push(Value value);
    Value pop();

    void defineNative(const std::string& name, NativeFn function);
    Value callFunction(Value callee, int argCount, Value* args);

private:
    bool handleError(const std::string& message);
    
    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    std::vector<TryHandler> tryHandlers;
    std::string currentScriptPath;
    
    std::unordered_map<std::string, Value> globals;
    std::vector<std::string> scriptArgs;  // CLI args passed to script
    
    // Open upvalues: maps stack index → shared upvalue box.
    // Ensures two closures capturing the same local share the same box.
    std::unordered_map<int, std::shared_ptr<ObjUpvalue>> openUpvalues;

    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    InterpretResult run(int targetDepth = 0);
};
